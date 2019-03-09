//
//  ASTClosure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 16/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Analysis/ThunkBuilder.hpp"
#include "Compiler.hpp"
#include "Functions/FunctionType.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

ASTClosure::ASTClosure(std::unique_ptr<Function> &&closure, const SourcePosition &p, bool isEscaping)
        : ASTExpr(p), closure_(std::move(closure)), isEscaping_(isEscaping) {}

Type ASTClosure::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    closure_->setClosure();
    analyser->configureClosure(closure_.get());
    analyser->semanticAnalyser()->analyseFunctionDeclaration(closure_.get());

    applyBoxingFromExpectation(analyser, expectation);

    auto scoper = std::make_unique<CapturingSemanticScoper>(analyser->scoper(), isEscaping_);
    auto scoperPtr = scoper.get();
    FunctionAnalyser closureAnaly(closure_.get(), std::move(scoper), analyser->semanticAnalyser());
    scoperPtr->setPathAnalyser(&closureAnaly.pathAnalyser());
    closureAnaly.analyse();
    capture_.captures = dynamic_cast<CapturingSemanticScoper &>(closureAnaly.scoper()).captures();
    if (closureAnaly.pathAnalyser().hasPotentially(PathAnalyserIncident::UsedSelf)) {
        analyser->checkThisUse(position());

        if (isEscaping_ && (analyser->typeContext().calleeType().type() == TypeType::ValueType ||
                            analyser->typeContext().calleeType().type() == TypeType::Enum)) {
            analyser->compiler()->error(CompilerError(position(),
                                                      "Escaping closure annot capture Value Type context in closure. "
                                                      "Create an explicit variable to copy a value."));
        }

        capture_.self = analyser->typeContext().calleeType();
    }
    return Type(closure_.get());
}

void ASTClosure::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyseAllocation(type);
    for (auto &capture : capture_.captures) {
        analyser->recordVariableGet(capture.sourceId, type);
    }
    if (capture_.capturesSelf()) {
        analyser->recordThis(MFFlowCategory::Escaping);
    }
    MFFunctionAnalyser(closure_.get()).analyse();
}

void ASTClosure::applyBoxingFromExpectation(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() != TypeType::Callable || expectation.parametersCount() != closure_->parameters().size()) {
        return;
    }

    auto expReturn = expectation.returnType();
    auto returnType = closure_->returnType()->type();
    if (returnType.compatibleTo(expReturn, analyser->typeContext())) {
        if (returnType.storageType() != expReturn.storageType()) {
            switch (expReturn.storageType()) {
                case StorageType::SimpleOptional:
                case StorageType::PointerOptional:
                    assert(returnType.storageType() == StorageType::Simple);
                    closure_->setReturnType(std::make_unique<ASTLiteralType>(returnType.optionalized()));
                    break;
                case StorageType::Simple:
                    // We cannot deal with this, can we?
                    break;
                case StorageType::Box: {
                    closure_->setReturnType(std::make_unique<ASTLiteralType>(returnType.boxedFor(expReturn.boxedFor())));
                    break;
                }
            }
        }
        else if (returnType.type() == TypeType::Box && !returnType.areMatchingBoxes(expReturn, analyser->typeContext())) {
            closure_->setReturnType(std::make_unique<ASTLiteralType>(returnType.unboxed().boxedFor(expReturn.boxedFor())));
        }
    }

    for (size_t i = 0; i < closure_->parameters().size(); i++) {
        auto &param = closure_->parameters()[i];
        auto &paramType = param.type->type();
        auto expParam = expectation.parameters()[i];
        if (paramType.compatibleTo(expParam, analyser->typeContext())) {
            if (paramType.storageType() != expParam.storageType()) {
                switch (expParam.storageType()) {
                    case StorageType::SimpleOptional:
                    case StorageType::PointerOptional:
                        assert(paramType.storageType() == StorageType::Simple);
                        closure_->setParameterType(i, std::make_unique<ASTLiteralType>(paramType.optionalized()));
                        break;
                    case StorageType::Simple:
                        // We cannot deal with this, can we?
                        break;
                    case StorageType::Box:
                        closure_->setParameterType(i, std::make_unique<ASTLiteralType>(paramType.boxedFor(expParam.boxedFor())));
                        break;
                }
            }
            else if (paramType.type() == TypeType::Box && !paramType.areMatchingBoxes(expParam, analyser->typeContext())) {
                closure_->setParameterType(i, std::make_unique<ASTLiteralType>(paramType.unboxed().boxedFor(expParam.boxedFor())));
            }
        }
    }
}

void ASTCallableBox::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyseAllocation(type);
    expr_->analyseMemoryFlow(analyser, type);
    
    MFFunctionAnalyser(thunk_.get()).analyse();
}

}  // namespace EmojicodeCompiler
