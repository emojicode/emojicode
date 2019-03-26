//
//  ASTControlFlow_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTControlFlow.hpp"
#include "Compiler.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

void ASTIf::generate(FunctionCodeGenerator *fg) const {
    auto *function = fg->builder().GetInsertBlock()->getParent();

    bool addAfter = false;
    auto afterIfBlock = llvm::BasicBlock::Create(fg->ctx(), "afterIf");

    for (size_t i = 0; i < conditions_.size(); i++) {
        auto thenBlock = llvm::BasicBlock::Create(fg->ctx(), "then", function);
        auto elseBlock = llvm::BasicBlock::Create(fg->ctx(), "else", function);

        auto cond = conditions_[i]->generate(fg);
        fg->releaseTemporaryObjects();
        fg->builder().CreateCondBr(cond, thenBlock, elseBlock);
        fg->builder().SetInsertPoint(thenBlock);
        blocks_[i].generate(fg);

        if (!blocks_[i].returnedCertainly()) {
            fg->builder().CreateBr(afterIfBlock);
            addAfter = true;
        }
        fg->builder().SetInsertPoint(elseBlock);
    }

    if (hasElse()) {
        blocks_.back().generate(fg);

        if (!blocks_.back().returnedCertainly()) {
            fg->builder().CreateBr(afterIfBlock);
            addAfter = true;
        }
    }
    else {
        fg->builder().CreateBr(afterIfBlock);
        addAfter = true;
    }

    if (addAfter) {
        function->getBasicBlockList().push_back(afterIfBlock);
        fg->builder().SetInsertPoint(afterIfBlock);
    }
}

void ASTRepeatWhile::generate(FunctionCodeGenerator *fg) const {
    auto *function = fg->builder().GetInsertBlock()->getParent();

    auto afterBlock = llvm::BasicBlock::Create(fg->ctx(), "afterRepeatWhile");
    auto whileCondBlock = llvm::BasicBlock::Create(fg->ctx(), "whileCond", function);
    auto repeatBlock = llvm::BasicBlock::Create(fg->ctx(), "repeat", function);

    fg->builder().CreateBr(whileCondBlock);

    fg->builder().SetInsertPoint(whileCondBlock);
    auto cond = condition_->generate(fg);
    fg->releaseTemporaryObjects();
    fg->builder().CreateCondBr(cond, repeatBlock, afterBlock);

    fg->builder().SetInsertPoint(repeatBlock);
    block_.generate(fg);

    if (!block_.returnedCertainly()) {
        fg->builder().CreateBr(whileCondBlock);
    }

    function->getBasicBlockList().push_back(afterBlock);
    fg->builder().SetInsertPoint(afterBlock);
}

void ASTErrorHandler::generate(FunctionCodeGenerator *fg) const {
    auto *function = fg->builder().GetInsertBlock()->getParent();

    auto afterBlock = llvm::BasicBlock::Create(fg->ctx(), "cont");
    auto noError = llvm::BasicBlock::Create(fg->ctx(), "noError", function);
    auto errorBlock = llvm::BasicBlock::Create(fg->ctx(), "error", function);

    auto errorDest = prepareErrorDestination(fg, value_.get());
    auto value = value_->generate(fg);
    auto tom = fg->takeTemporaryObjectsManager();

    fg->builder().CreateCondBr(isError(fg, errorDest), errorBlock, noError);

    fg->builder().SetInsertPoint(errorBlock);
    tom.releaseTemporaryObjects(fg, false, value_->producesTemporaryObject());
    fg->setVariable(errorVar_, fg->builder().CreateLoad(errorDest));
    errorBlock_.generate(fg);
    if (!errorBlock_.returnedCertainly()) {
        fg->builder().CreateBr(afterBlock);
    }

    fg->builder().SetInsertPoint(noError);
    tom.releaseTemporaryObjects(fg, true, false);
    if (!valueVarName_.empty()) {
        fg->setVariable(valueVar_, value);
    }
    valueBlock_.generate(fg);
    if (!valueBlock_.returnedCertainly()) {
        fg->builder().CreateBr(afterBlock);
    }

    if (!errorBlock_.returnedCertainly() || !valueBlock_.returnedCertainly()) {
        function->getBasicBlockList().push_back(afterBlock);
        fg->builder().SetInsertPoint(afterBlock);
    }
}

void ASTForIn::generate(FunctionCodeGenerator *fg) const {
    block_.generate(fg);
}

}  // namespace EmojicodeCompiler
