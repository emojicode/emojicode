//
//  ASTExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/TypeExpectation.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"

namespace EmojicodeCompiler {

void ASTUnaryMFForwarding::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    expr_->analyseMemoryFlow(analyser, type);
}

Type ASTTypeAsValue::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    auto &type = type_->analyseType(analyser->typeContext());
    ASTTypeValueType::checkTypeValue(tokenType_, type, analyser->typeContext(), position());
    return Type(MakeTypeAsValue, type);
}

Type ASTSizeOf::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    type_->analyseType(analyser->typeContext());
    return analyser->integer();
}

Type ASTConditionalAssignment::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (t.unboxedType() != TypeType::Optional) {
        throw CompilerError(position(), "Condition assignment can only be used with optionals.");
    }

    t = t.optionalType();

    auto &variable = analyser->scoper().currentScope().declareVariable(varName_, t, true, position());
    variable.initialize();
    varId_ = variable.id();

    return analyser->boolean();
}

void ASTConditionalAssignment::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->recordVariableSet(varId_, expr_.get(), expr_->expressionType().optionalType());
    analyser->take(expr_.get());
}

Type ASTSuper::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    if (isSuperconstructorRequired(analyser->function()->functionType())) {
        analyseSuperInit(analyser);
        return Type::noReturn();
    }
    if (analyser->function()->functionType() != FunctionType::ObjectMethod) {
        throw CompilerError(position(), "Not within an object-context.");
    }

    Class *superclass = analyser->typeContext().calleeType().klass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(position(), "Class has no superclass.");
    }

    function_ = superclass->getMethod(name_, Type(superclass), analyser->typeContext(),
                                             args_.isImperative(), position());
    calleeType_ = Type(superclass);
    return analyser->analyseFunctionCall(&args_, calleeType_, function_);
}

void ASTSuper::analyseSuperInit(FunctionAnalyser *analyser) {
    if (analyser->typeContext().calleeType().klass()->superclass() == nullptr) {
        throw CompilerError(position(), "Class does not have a super class");
    }
    if (analyser->pathAnalyser().hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
        analyser->compiler()->error(CompilerError(position(), "Superinitializer might have already been called."));
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

void ASTSuper::analyseSuperInitErrorProneness(FunctionAnalyser *analyser, const Initializer *initializer) {
    if (initializer->errorProne()) {
        auto thisInitializer = dynamic_cast<Initializer*>(analyser->function());
        if (!thisInitializer->errorProne()) {
            throw CompilerError(position(), "Cannot call an error-prone super initializer in a non ",
                                "error-prone initializer.");
        }
        if (!initializer->errorType()->type().identicalTo(thisInitializer->errorType()->type(), analyser->typeContext(),
                                                          nullptr)) {
            throw CompilerError(position(), "Super initializer must have same error enum type.");
        }
        manageErrorProneness_ = true;
        analyseInstanceVariables(analyser);
    }
}

Type ASTCallableCall::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false), &callable_);
    if (type.type() != TypeType::Callable) {
        throw CompilerError(position(), "Given value is not callable.");
    }
    for (size_t i = 1; i < type.genericArguments().size(); i++) {
        analyser->expectType(type.genericArguments()[i], &args_.args()[i - 1]);
    }
    return type.genericArguments()[0];
}

void ASTCallableCall::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    callable_->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    for (auto &arg : args_.args()) {
        analyser->take(arg.get());
        arg->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);  // We cannot at all say what the callable will do.
    }
}

}  // namespace EmojicodeCompiler
