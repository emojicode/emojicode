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

    auto afterIfBlock = llvm::BasicBlock::Create(fg->generator()->context(), "afterIf");
    for (size_t i = 0; i < conditions_.size(); i++) {
        auto thenBlock = llvm::BasicBlock::Create(fg->generator()->context(), "then", function);
        auto elseBlock = llvm::BasicBlock::Create(fg->generator()->context(), "else", function);

        auto cond = conditions_[i]->generate(fg);
        fg->releaseTemporaryObjects();
        fg->builder().CreateCondBr(cond, thenBlock, elseBlock);
        fg->builder().SetInsertPoint(thenBlock);
        blocks_[i].generate(fg);

        if (!blocks_[i].returnedCertainly()) {
            fg->builder().CreateBr(afterIfBlock);
        }
        fg->builder().SetInsertPoint(elseBlock);
    }

    if (hasElse()) {
        blocks_.back().generate(fg);
    }

    if (!blocks_.back().returnedCertainly()) {
        fg->builder().CreateBr(afterIfBlock);
        function->getBasicBlockList().push_back(afterIfBlock);
        fg->builder().SetInsertPoint(afterIfBlock);
    }
}

void ASTRepeatWhile::generate(FunctionCodeGenerator *fg) const {
    auto *function = fg->builder().GetInsertBlock()->getParent();

    auto afterBlock = llvm::BasicBlock::Create(fg->generator()->context(), "afterRepeatWhile");
    auto whileCondBlock = llvm::BasicBlock::Create(fg->generator()->context(), "whileCond", function);
    auto repeatBlock = llvm::BasicBlock::Create(fg->generator()->context(), "repeat", function);

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

    auto afterBlock = llvm::BasicBlock::Create(fg->generator()->context(), "cont");
    auto noError = llvm::BasicBlock::Create(fg->generator()->context(), "noError", function);
    auto errorBlock = llvm::BasicBlock::Create(fg->generator()->context(), "error", function);

    auto error = value_->generate(fg);
    auto isError = valueIsBoxed_ ? fg->buildHasNoValueBox(error) : fg->buildGetIsError(error);

    fg->builder().CreateCondBr(isError, errorBlock, noError);

    fg->builder().SetInsertPoint(errorBlock);
    llvm::Value *err;
    if (valueIsBoxed_) {
        auto alloca = fg->createEntryAlloca(fg->typeHelper().box());
        fg->builder().CreateStore(error, alloca);
        err = fg->buildErrorEnumValueBoxPtr(alloca, value_->expressionType().errorEnum());
    }
    else {
        err = fg->builder().CreateExtractValue(error, 0);
    }
    fg->scoper().getVariable(errorVar_) = LocalVariable(false, err);
    errorBlock_.generate(fg);
    if (!errorBlock_.returnedCertainly()) {
        fg->builder().CreateBr(afterBlock);
    }

    fg->builder().SetInsertPoint(noError);
    auto value = valueIsBoxed_ ? error : fg->builder().CreateExtractValue(error, 1);
    fg->scoper().getVariable(valueVar_) = LocalVariable(false, value);
    valueBlock_.generate(fg);
    if (!valueBlock_.returnedCertainly()) {
        fg->builder().CreateBr(afterBlock);
    }

    function->getBasicBlockList().push_back(afterBlock);
    fg->builder().SetInsertPoint(afterBlock);
}

void ASTForIn::generate(FunctionCodeGenerator *fg) const {
    block_.generate(fg);
}

}  // namespace EmojicodeCompiler
