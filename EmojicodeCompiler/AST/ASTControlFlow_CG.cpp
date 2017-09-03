//
//  ASTControlFlow_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTControlFlow.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

void ASTIf::generate(FnCodeGenerator *fncg) const {
    // This list will contain all placeholders that need to be replaced with a relative jump value
    // to jump past the whole if
    auto endPlaceholders = std::vector<std::pair<InstructionCount, FunctionWriterPlaceholder>>();

    std::experimental::optional<FunctionWriterCountPlaceholder> placeholder;
    for (size_t i = 0; i < conditions_.size(); i++) {
        if (placeholder) {
            fncg->wr().writeInstruction(INS_JUMP_FORWARD);
            auto endPlaceholder = fncg->wr().writeInstructionPlaceholder();
            endPlaceholders.emplace_back(fncg->wr().count(), endPlaceholder);
            placeholder->write();
        }

        fncg->scoper().pushScope();
        conditions_[i]->generate(fncg);
        fncg->wr().writeInstruction(INS_JUMP_FORWARD_IF_NOT);
        placeholder = fncg->wr().writeInstructionsCountPlaceholderCoin();
        fncg->scoper().pushScope();
        blocks_[i].generate(fncg);
        fncg->scoper().popScope(0);
    }

    if (hasElse()) {
        fncg->wr().writeInstruction(INS_JUMP_FORWARD);
        auto elseCountPlaceholder = fncg->wr().writeInstructionsCountPlaceholderCoin();
        placeholder->write();
        fncg->scoper().pushScope();
        blocks_.back().generate(fncg);
        fncg->scoper().popScope(0);
        elseCountPlaceholder.write();
    }
    else {
        placeholder->write();
    }

    for (auto endPlaceholder : endPlaceholders) {
        endPlaceholder.second.write(fncg->wr().count() - endPlaceholder.first);
    }
}

void ASTRepeatWhile::generate(FnCodeGenerator *fncg) const {
    fncg->scoper().pushScope();
    fncg->wr().writeInstruction(INS_JUMP_FORWARD);
    auto placeholder = fncg->wr().writeInstructionsCountPlaceholderCoin();

    auto delta = fncg->wr().count();
    block_.generate(fncg);
    placeholder.write();
    condition_->generate(fncg);

    fncg->wr().writeInstruction(INS_JUMP_BACKWARD_IF);
    fncg->wr().writeInstruction(fncg->wr().count() - delta + 1);
    fncg->scoper().popScope(fncg->wr().count());
}

void ASTErrorHandler::generate(FnCodeGenerator *fncg) const {
    value_->generate(fncg);
    fncg->scoper().pushScope();
    auto &var = fncg->scoper().declareVariable(varId_, value_->expressionType());
    fncg->copyToVariable(var.stackIndex, false, value_->expressionType());
    fncg->pushVariableReference(var.stackIndex, false);
    fncg->wr().writeInstruction(INS_IS_ERROR);
    fncg->wr().writeInstruction(INS_JUMP_FORWARD_IF);
    auto valueBlockCount = fncg->wr().writeInstructionsCountPlaceholderCoin();
    if (!valueIsBoxed_) {
        var.stackIndex.increment();
    }
    var.type = valueType_;
    valueBlock_.generate(fncg);
    fncg->wr().writeInstruction(INS_JUMP_FORWARD);
    auto errorBlockCount = fncg->wr().writeInstructionsCountPlaceholderCoin();
    valueBlockCount.write();
    if (valueIsBoxed_) {
        var.stackIndex.increment();
    }
    var.type = value_->expressionType().genericArguments()[0];
    errorBlock_.generate(fncg);
    errorBlockCount.write();
    fncg->scoper().popScope(fncg->wr().count());
}

void ASTForIn::generate(FnCodeGenerator *fncg) const {
    fncg->scoper().pushScope();

    auto callCG = CallCodeGenerator(fncg, INS_DISPATCH_PROTOCOL);

    callCG.generate(*iteratee_, iteratee_->expressionType(), ASTArguments(position()), std::u32string(1, E_DANGO));

    auto &itVar = fncg->scoper().declareVariable(iteratorVar_, Type(PR_ENUMERATOR, false));
    auto &elementVar = fncg->scoper().declareVariable(elementVar_, elementType_);

    fncg->copyToVariable(itVar.stackIndex, false, Type(PR_ENUMERATEABLE, false));

    fncg->wr().writeInstruction(INS_JUMP_FORWARD);
    auto placeholder = fncg->wr().writeInstructionsCountPlaceholderCoin();

    auto getVar = ASTProxyExpr(position(), itVar.type, [&itVar](auto *fncg) {
        fncg->pushVariableReference(itVar.stackIndex, false);
    });

    auto delta = fncg->wr().count();
    callCG.generate(getVar, itVar.type, ASTArguments(position()), std::u32string(1, 0x1F53D));
    fncg->copyToVariable(elementVar.stackIndex, false, Type(PR_ENUMERATEABLE, false));
    block_.generate(fncg);
    placeholder.write();

    callCG.generate(getVar, itVar.type, ASTArguments(position()), std::u32string(1, E_RED_QUESTION_MARK));
    fncg->wr().writeInstruction(INS_JUMP_BACKWARD_IF);
    fncg->wr().writeInstruction(fncg->wr().count() - delta + 1);
    fncg->scoper().popScope(fncg->wr().count());
}

}  // namespace EmojicodeCompiler
