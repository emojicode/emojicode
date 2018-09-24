//
//  ASTControlFlow.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Types/Class.hpp"
#include "AST/ASTNode.hpp"
#include "ASTControlFlow.hpp"
#include "ASTMethod.hpp"
#include "ASTVariables.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Types/Protocol.hpp"
#include "Emojis.h"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
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

        analyser->pathAnalyser().endMutualExclusiveBranches();
    }
    else {
        analyser->pathAnalyser().endUncertainBranches();
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
    analyser->pathAnalyser().endUncertainBranches();
}

void ASTRepeatWhile::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    condition_->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    block_.analyseMemoryFlow(analyser);
    analyser->popScope(&block_);
}

void ASTErrorHandler::analyse(FunctionAnalyser *analyser) {
    Type type = analyser->expect(TypeExpectation(false, false), &value_);

    if (type.unboxedType() != TypeType::Error) {
        throw CompilerError(position(), "🥑 can only be used with 🚨.");
    }

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();

    valueIsBoxed_ = type.storageType() == StorageType::Box;
    valueType_ = type.errorType();
    auto &var = analyser->scoper().currentScope().declareVariable(valueVarName_, valueType_, true, position());
    var.initialize();
    valueVar_ = var.id();
    valueBlock_.analyse(analyser);
    valueBlock_.popScope(analyser);
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    auto &errVar = analyser->scoper().currentScope().declareVariable(errorVarName_, type.errorEnum(), true, position());
    errVar.initialize();
    errorVar_ = errVar.id();
    errorBlock_.analyse(analyser);
    errorBlock_.popScope(analyser);
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().endMutualExclusiveBranches();
}

void ASTErrorHandler::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    analyser->recordVariableSet(valueVar_, value_.get(), valueType_);
    valueBlock_.analyseMemoryFlow(analyser);
    analyser->popScope(&valueBlock_);
    analyser->recordVariableSet(errorVar_, nullptr, value_->expressionType().errorEnum());
    errorBlock_.analyseMemoryFlow(analyser);
    analyser->popScope(&errorBlock_);
}

void ASTForIn::analyse(FunctionAnalyser *analyser) {
    analyser->scoper().pushScope();

    ASTBlock newBlock(position());

    auto getIterator = std::make_shared<ASTMethod>(std::u32string(1, E_DANGO), std::move(iteratee_),
                                                   ASTArguments(position()), position());
    newBlock.appendNode(std::make_unique<ASTConstantVariable>(U"iterator", getIterator, position()));
    auto getNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(U"iterator", position()),
                                               ASTArguments(position()), position());
    block_.prependNode(std::make_unique<ASTConstantVariable>(varName_, getNext, position()));

    auto hasNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(U"iterator", position()),
                                               ASTArguments(position(), false), position());
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
