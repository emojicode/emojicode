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
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Types/Protocol.hpp"
#include "ASTMethod.hpp"
#include "ASTVariables.hpp"

namespace EmojicodeCompiler {

void ASTIf::analyse(FunctionAnalyser *analyser) {
    for (size_t i = 0; i < conditions_.size(); i++) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        analyser->expectType(analyser->boolean(), &conditions_[i]);
        blocks_[i].analyse(analyser);
        analyser->scoper().popScope(analyser->compiler());
        analyser->pathAnalyser().endBranch();
    }

    if (hasElse()) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        blocks_.back().analyse(analyser);
        analyser->scoper().popScope(analyser->compiler());
        analyser->pathAnalyser().endBranch();

        analyser->pathAnalyser().endMutualExclusiveBranches();
    }
    else {
        analyser->pathAnalyser().endUncertainBranches();
    }
}

void ASTRepeatWhile::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    analyser->expectType(analyser->boolean(), &condition_);
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endUncertainBranches();
}

void ASTErrorHandler::analyse(FunctionAnalyser *analyser) {
    Type type = analyser->expect(TypeExpectation(false, false), &value_);

    if (type.unboxedType() != TypeType::Error) {
        throw CompilerError(position(), "🥑 can only be used with 🚨.");
    }

    analyser->scoper().pushScope();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();

    valueIsBoxed_ = type.storageType() == StorageType::Box;
    valueType_ = type.errorType();
    auto &var = analyser->scoper().currentScope().declareVariable(valueVarName_, valueType_, true, position());
    var.initialize();
    valueVar_ = var.id();
    valueBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    auto &errVar = analyser->scoper().currentScope().declareVariable(errorVarName_, type.errorEnum(), true, position());
    errVar.initialize();
    errorVar_ = errVar.id();
    errorBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().endMutualExclusiveBranches();
    analyser->scoper().popScope(analyser->compiler());
}

void ASTForIn::analyse(FunctionAnalyser *analyser) {
    analyser->scoper().pushScope();

    ASTBlock newBlock(position());

    auto getIterator = std::make_shared<ASTMethod>(std::u32string(1, E_DANGO), std::move(iteratee_),
                                                   ASTArguments(position()), position());
    newBlock.appendNode(std::make_shared<ASTConstantVariable>(U"iterator", getIterator, position()));
    auto getNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(U"iterator", position()),
                                               ASTArguments(position()), position());
    block_.preprendNode(std::make_shared<ASTConstantVariable>(varName_, getNext, position()));

    auto hasNext = std::make_shared<ASTMethod>(std::u32string(1, 0x1F53D),
                                               std::make_shared<ASTGetVariable>(U"iterator", position()),
                                               ASTArguments(position(), false), position());
    newBlock.appendNode(std::make_shared<ASTRepeatWhile>(hasNext, std::move(block_), position()));
    block_ = newBlock;
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
}

}  // namespace EmojicodeCompiler
