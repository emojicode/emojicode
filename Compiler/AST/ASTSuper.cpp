//
//  ASTSuper.cpp
//  runtime
//
//  Created by Theo Weidmann on 07.03.19.
//

#include "ASTSuper.hpp"
#include "Functions/Initializer.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Types/Class.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"

namespace EmojicodeCompiler {

Type ASTSuper::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (isSuperconstructorRequired(analyser->functionType())) {
        analyseSuperInit(analyser);
        return Type::noReturn();
    }
    if (analyser->functionType() != FunctionType::ObjectMethod) {
        throw CompilerError(position(), "Not within an object-context.");
    }

    Class *superclass = analyser->typeContext().calleeType().klass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(position(), "Class has no superclass.");
    }

    function_ = superclass->getMethod(name_, Type(superclass), analyser->typeContext(),
                                      args_.mood(), position());
    calleeType_ = Type(superclass);
    return analyser->analyseFunctionCall(&args_, calleeType_, function_);
}

void ASTSuper::analyseSuperInit(ExpressionAnalyser *analyser) {
    if (analyser->typeContext().calleeType().klass()->superclass() == nullptr) {
        throw CompilerError(position(), "Class does not have a super class");
    }
    if (analyser->pathAnalyser().hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
        analyser->error(CompilerError(position(), "Superinitializer might have already been called."));
    }

    analyser->scoper().instanceScope()->uninitializedVariablesCheck(position(), "Instance variable \"", "\" must be "
                                                                    "initialized before calling the superinitializer.");

    init_ = true;
    auto eclass = analyser->typeContext().calleeType().klass();
    auto initializer = eclass->superclass()->getInitializer(name_, Type(eclass),
                                                            analyser->typeContext(), position());
    calleeType_ = Type(eclass->superclass());
    analyser->analyseFunctionCall(&args_, calleeType_, initializer);
    analyseSuperInitErrorProneness(analyser, initializer);
    function_ = initializer;
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::CalledSuperInitializer);
}

void ASTSuper::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->recordThis(function_->memoryFlowTypeForThis());
    analyser->analyseFunctionCall(&args_, nullptr, function_);
}

const Type& ASTSuper::errorType() const {
    return function_->errorType()->type();
}

bool ASTSuper::isErrorProne() const {
    return !init_ && function_->errorProne();
}

void ASTSuper::analyseSuperInitErrorProneness(ExpressionAnalyser *eanalyser, const Initializer *initializer) {
    auto analyser = dynamic_cast<FunctionAnalyser *>(eanalyser);
    if (initializer->errorProne()) {
        auto thisInitializer = dynamic_cast<const Initializer*>(analyser->function());
        if (!thisInitializer->errorProne()) {
            throw CompilerError(position(), "Cannot call an error-prone super initializer in a non ",
                                "error-prone initializer.");
        }
        if (!initializer->errorType()->type().compatibleTo(thisInitializer->errorType()->type(),
                                                           analyser->typeContext())) {
            throw CompilerError(position(), "Super initializer may raise ",
                                initializer->errorType()->type().toString(analyser->typeContext()),
                                " which cannot be reraised from this initializer as it is not compatible to ",
                                thisInitializer->errorType()->type().toString(analyser->typeContext()), ".");
        }
        manageErrorProneness_ = true;
        analyseInstanceVariables(analyser);
    }
}

Value* ASTSuper::generate(FunctionCodeGenerator *fg) const {
    auto castedThis = fg->builder().CreateBitCast(fg->thisValue(), fg->typeHelper().llvmTypeFor(calleeType_));
    auto ret = CallCodeGenerator(fg, CallType::StaticDispatch).generate(castedThis, calleeType_, args_, function_,
                                                                        manageErrorProneness_ ? fg->errorPointer() :
                                                                        errorPointer());
    if (manageErrorProneness_) {
        fg->createIfElseBranchCond(isError(fg, fg->errorPointer()), [&]() {  // TODO: finish
            buildDestruct(fg);
            fg->builder().CreateRet(llvm::UndefValue::get(fg->llvmReturnType()));
            return false;
        }, [] { return true; });
    }

    return init_ ? nullptr : ret;
}

}  // namespace EmojicodeCompiler
