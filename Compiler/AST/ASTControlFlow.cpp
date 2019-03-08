//
//  ASTControlFlow.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTControlFlow.hpp"
#include "AST/ASTNode.hpp"
#include "ASTMethod.hpp"
#include "ASTVariables.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Emojis.h"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

void ASTIf::analyse(FunctionAnalyser *analyser) {
    for (size_t i = 0; i < conditions_.size(); i++) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        analyser->expectType(analyser->boolean(), &conditions_[i]);
        blocks_[i].analyse(analyser);
        blocks_[i].popScope(analyser);
        analyser->pathAnalyser().endBranch();
    }

    if (hasElse()) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        blocks_.back().analyse(analyser);
        blocks_.back().popScope(analyser);
        analyser->pathAnalyser().endBranch();

        analyser->pathAnalyser().finishMutualExclusiveBranches();
    }
    else {
        analyser->pathAnalyser().finishUncertainBranches();
    }
}

void ASTIf::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    for (size_t i = 0; i < conditions_.size(); i++) {
        conditions_[i]->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
        blocks_[i].analyseMemoryFlow(analyser);
        analyser->popScope(&blocks_[i]);
    }
    if (hasElse()) {
        blocks_.back().analyseMemoryFlow(analyser);
    }
}

void ASTRepeatWhile::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    analyser->expectType(analyser->boolean(), &condition_);
    block_.analyse(analyser);
    block_.popScope(analyser);
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().finishUncertainBranches();
}

void ASTRepeatWhile::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    condition_->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    block_.analyseMemoryFlow(analyser);
    analyser->popScope(&block_);
}

void ASTErrorHandler::analyse(FunctionAnalyser *analyser) {
    auto call = dynamic_cast<ASTCall *>(value_.get());
    if (call == nullptr) {
        throw CompilerError(position(), "Expression is not a call.");
    }
    call->setHandledError();

    valueType_ = analyser->expect(TypeExpectation(false, false), &value_);

    if (!call->isErrorProne()) {
        throw CompilerError(position(), "Provided call is not error-prone.");
    }

    if (valueType_.type() == TypeType::NoReturn && !valueVarName_.empty()) {
        analyser->compiler()->error(CompilerError(position(),
                                                  "Call does not return a value but variable was provided."));
    }

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();

    if (!valueVarName_.empty()) {
        auto &var = analyser->scoper().currentScope().declareVariable(valueVarName_, valueType_, true, position());
        analyser->pathAnalyser().record(PathAnalyserIncident(false, var.id()));
        valueVar_ = var.id();
    }
    valueBlock_.analyse(analyser);
    valueBlock_.popScope(analyser);
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    errorType_ = call->errorType();
    auto &errVar = analyser->scoper().currentScope().declareVariable(errorVarName_, call->errorType(), true, position());
    analyser->pathAnalyser().record(PathAnalyserIncident(false, errVar.id()));
    errorVar_ = errVar.id();
    errorBlock_.analyse(analyser);
    errorBlock_.popScope(analyser);
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().finishMutualExclusiveBranches();
}

void ASTErrorHandler::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    analyser->take(value_.get());
    if (!valueVarName_.empty()) {
        analyser->recordVariableSet(valueVar_, value_.get(), valueType_);
    }
    valueBlock_.analyseMemoryFlow(analyser);
    analyser->popScope(&valueBlock_);
    analyser->recordVariableSet(errorVar_, nullptr, errorType_);
    errorBlock_.analyseMemoryFlow(analyser);
    analyser->popScope(&errorBlock_);
}

void ASTForIn::analyse(FunctionAnalyser *analyser) {
    analyser->scoper().pushScope();

    ASTBlock newBlock(position());

    auto iteratorVar = U"iterator" + varName_;

    auto getIterator = std::make_shared<ASTMethod>(std::u32string(1, E_DANGO), std::move(iteratee_),
                                                   ASTArguments(position()), position());
    newBlock.appendNode(std::make_unique<ASTConstantVariable>(iteratorVar, getIterator, position()));
    auto getNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(iteratorVar, position()),
                                               ASTArguments(position()), position());
    block_.prependNode(std::make_unique<ASTConstantVariable>(varName_, getNext, position()));

    auto hasNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(iteratorVar, position()),
                                               ASTArguments(position(), Mood::Interogative), position());
    newBlock.appendNode(std::make_unique<ASTRepeatWhile>(hasNext, std::move(block_), position()));
    block_ = std::move(newBlock);
    block_.analyse(analyser);
    block_.popScope(analyser);
}

void ASTForIn::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    block_.analyseMemoryFlow(analyser);
    analyser->popScope(&block_);
}

}  // namespace EmojicodeCompiler
