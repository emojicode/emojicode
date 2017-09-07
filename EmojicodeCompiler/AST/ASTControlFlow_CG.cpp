//
//  ASTControlFlow_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTControlFlow.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

void ASTIf::generate(FunctionCodeGenerator *fncg) const {
    auto *function = fncg->builder().GetInsertBlock()->getParent();

    auto afterIfBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "afterIf");
    for (size_t i = 0; i < conditions_.size(); i++) {
        auto thenBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "then", function);
        auto elseBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "else", function);

        fncg->builder().CreateCondBr(conditions_[i]->generate(fncg), thenBlock, elseBlock);
        fncg->builder().SetInsertPoint(thenBlock);
        blocks_[i].generate(fncg);

        if (!certainlyReturned_) {
            fncg->builder().CreateBr(afterIfBlock);
        }
        fncg->builder().SetInsertPoint(elseBlock);
    }

    if (hasElse()) {
        blocks_.back().generate(fncg);
    }

    if (!certainlyReturned_) {
        fncg->builder().CreateBr(afterIfBlock);
        function->getBasicBlockList().push_back(afterIfBlock);
        fncg->builder().SetInsertPoint(afterIfBlock);
    }
}

void ASTRepeatWhile::generate(FunctionCodeGenerator *fncg) const {
    auto *function = fncg->builder().GetInsertBlock()->getParent();

    auto afterBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "afterRepeatWhile");
    auto whileCondBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "whileCond", function);
    auto repeatBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "repeat", function);

    fncg->builder().CreateBr(whileCondBlock);

    fncg->builder().SetInsertPoint(whileCondBlock);
    fncg->builder().CreateCondBr(condition_->generate(fncg), repeatBlock, afterBlock);

    fncg->builder().SetInsertPoint(repeatBlock);
    block_.generate(fncg);
    fncg->builder().CreateBr(whileCondBlock);

    function->getBasicBlockList().push_back(afterBlock);
    fncg->builder().SetInsertPoint(afterBlock);
}

void ASTErrorHandler::generate(FunctionCodeGenerator *fncg) const {
    // TODO: implement
//    value_->generate(fncg);
//    fncg->scoper().pushScope();
//    auto &var = fncg->scoper().declareVariable(varId_, value_->expressionType());
//    fncg->copyToVariable(var.stackIndex, false, value_->expressionType());
//    fncg->pushVariableReference(var.stackIndex, false);
//    fncg->wr().writeInstruction(INS_IS_ERROR);
//    fncg->wr().writeInstruction(INS_JUMP_FORWARD_IF);
//    auto valueBlockCount = fncg->wr().writeInstructionsCountPlaceholderCoin();
//    if (!valueIsBoxed_) {
//        var.stackIndex.increment();
//    }
//    var.type = valueType_;
//    valueBlock_.generate(fncg);
//    fncg->wr().writeInstruction(INS_JUMP_FORWARD);
//    auto errorBlockCount = fncg->wr().writeInstructionsCountPlaceholderCoin();
//    valueBlockCount.write();
//    if (valueIsBoxed_) {
//        var.stackIndex.increment();
//    }
//    var.type = value_->expressionType().genericArguments()[0];
//    errorBlock_.generate(fncg);
//    errorBlockCount.write();
//    fncg->scoper().popScope(fncg->wr().count());
}

void ASTForIn::generate(FunctionCodeGenerator *fncg) const {
    // TODO: implement
//    fncg->scoper().pushScope();
//
//    auto callCG = CallCodeGenerator(fncg, INS_DISPATCH_PROTOCOL);
//
//    callCG.generate(*iteratee_, iteratee_->expressionType(), ASTArguments(position()), std::u32string(1, E_DANGO));
//
//    auto &itVar = fncg->scoper().declareVariable(iteratorVar_, Type(PR_ENUMERATOR, false));
//    auto &elementVar = fncg->scoper().declareVariable(elementVar_, elementType_);
//
//    fncg->copyToVariable(itVar.stackIndex, false, Type(PR_ENUMERATEABLE, false));
//
//    fncg->wr().writeInstruction(INS_JUMP_FORWARD);
//    auto placeholder = fncg->wr().writeInstructionsCountPlaceholderCoin();
//
//    auto getVar = ASTProxyExpr(position(), itVar.type, [&itVar](auto *fncg) {
//        fncg->pushVariableReference(itVar.stackIndex, false);
//    });
//
//    auto delta = fncg->wr().count();
//    callCG.generate(getVar, itVar.type, ASTArguments(position()), std::u32string(1, 0x1F53D));
//    fncg->copyToVariable(elementVar.stackIndex, false, Type(PR_ENUMERATEABLE, false));
//    block_.generate(fncg);
//    placeholder.write();
//
//    callCG.generate(getVar, itVar.type, ASTArguments(position()), std::u32string(1, E_RED_QUESTION_MARK));
//    fncg->wr().writeInstruction(INS_JUMP_BACKWARD_IF);
//    fncg->wr().writeInstruction(fncg->wr().count() - delta + 1);
//    fncg->scoper().popScope(fncg->wr().count());
}

}  // namespace EmojicodeCompiler
