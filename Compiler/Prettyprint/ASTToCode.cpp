//
//  ASTToCode.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 22/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Parsing/OperatorHelper.hpp"
#include "AST/ASTBinaryOperator.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTControlFlow.hpp"
#include "AST/ASTExpr.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTUnary.hpp"
#include "AST/ASTUnsafeBlock.hpp"
#include "AST/ASTVariables.hpp"
#include "PrettyStream.hpp"
#include "Types/Type.hpp"
#include <sstream>

namespace EmojicodeCompiler {

void ASTArguments::toCode(PrettyStream &pretty) const {
    if (!arguments_.empty()) {
        pretty << " ";
        for (auto &arg : arguments_) {
            arg->toCode(pretty);
        }
    }
    pretty.refuseOffer() << (imperative_ ? "❗️" : "❓️");
}

void ASTBlock::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    if (stmts_.empty()) {
        pretty << "🍇🍉\n";
        return;
    }
    pretty << "🍇";
    pretty.offerNewLine();
    innerToCode(pretty);
    pretty.indent() << "🍉\n";
}

void ASTBlock::innerToCode(PrettyStream &pretty) const {
    pretty.increaseIndent();
    for (auto &stmt : stmts_) {
        stmt->toCode(pretty);
        pretty.offerNewLine();
        if (stmt->paragraph()) {
            pretty << "\n";
        }
    }
    pretty.decreaseIndent();
}

void ASTRepeatWhile::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🔁 ";
    condition_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTForIn::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🔂 " << utf8(varName_) << " ";
    iteratee_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTUnsafeBlock::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "☣️ ";
    block_.toCode(pretty);
}

void ASTIf::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "↪️ ";
    conditions_.front()->toCode(pretty);
    pretty << " ";
    blocks_.front().toCode(pretty);
    for (size_t i = 1; i < conditions_.size(); i++) {
        pretty.indent() << "🙅↪️ ";
        conditions_[i]->toCode(pretty);
        pretty << " ";
        blocks_[i].toCode(pretty);
    }
    if (hasElse()) {
        pretty.indent() << "🙅 ";
        blocks_.back().toCode(pretty);
    }
}

void ASTClosure::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.printClosure(closure_.get());
}

void ASTErrorHandler::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🥑 " << utf8(valueVarName_) << " ";
    value_->toCode(pretty);
    pretty << " ";
    valueBlock_.toCode(pretty);
    pretty.indent() << "🙅‍♀️ " << utf8(errorVarName_) << " ";
    errorBlock_.toCode(pretty);
}

void ASTExprStatement::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
}

void ASTVariableDeclaration::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🖍🆕 " << utf8(varName_) << " " << type_;
}

void ASTVariableAssignment::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
    pretty << "➡️ 🖍" << utf8(name());
}

void ASTVariableDeclareAndAssign::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
    pretty << "➡️ 🖍🆕 " << utf8(name());
}

void ASTConstantVariable::toCode(PrettyStream &pretty) const {
    pretty.indent();
    expr_->toCode(pretty);
    pretty.printComments(position());
    pretty << " ➡️ " << utf8(name());
}

void ASTConditionalAssignment::toCode(PrettyStream &pretty) const {
    expr_->toCode(pretty);
    pretty << " ➡️ " << utf8(varName_);
}

void ASTOperatorAssignment::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    auto binaryOperator = dynamic_cast<ASTBinaryOperator *>(expr_.get());
    pretty << utf8(name()) << " ⬅️" << utf8(operatorName(binaryOperator->operatorType())) << " ";
    binaryOperator->right()->toCode(pretty);
}

void ASTGetVariable::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << utf8(name());
}

void ASTSuper::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "⤴️" << utf8(name_);
    args_.toCode(pretty);
}

void ASTInitialization::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🆕";
    typeExpr_->toCode(pretty);
    pretty << utf8(name_);
    args_.toCode(pretty);
}

void ASTThisType::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🐕";
}

void ASTInferType::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "⚫️";
}

void ASTStaticType::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << type_;
}

void ASTTypeFromExpr::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "⬛️";
    expr_->toCode(pretty);
}

void ASTTypeAsValue::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << ASTTypeValueType::toString(tokenType_) << type_;
}

void ASTSizeOf::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "⚖️" << type_;
}

void ASTCallableCall::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "⁉️";
    callable_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTBooleanTrue::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "👍";
}

void ASTBooleanFalse::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "👎";
}

void ASTThis::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🐕";
}

void ASTIsError::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🚥";
    value_->toCode(pretty);
}

void ASTUnwrap::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << " 🍺";
    value_->toCode(pretty);
}

void ASTNumberLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << utf8(string_);
    pretty.offerSpace();
}

void ASTSymbolLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🔟" << utf8(std::u32string(1, value_));
}

void ASTNoValue::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🤷‍♀️";
}

void ASTStringLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🔤" << utf8(value_) << "🔤";
}

void ASTRaise::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🚨";
    value_->toCode(pretty);
}

void ASTReturn::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    if (value_ == nullptr) {
        pretty.indent() << "↩️↩️";
    }
    else {
        pretty.indent() << "↩️ ";
        value_->toCode(pretty);
    }
}

void ASTCast::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🔲";
    value_->toCode(pretty);
    typeExpr_->toCode(pretty);
}

void ASTMethod::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << utf8(name_);
    callee_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTConcatenateLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🍪 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍪";
}

void ASTListLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🍨 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTDictionaryLiteral::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    pretty << "🍯 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTBinaryOperator::printBinaryOperand(int precedence, const std::shared_ptr<ASTExpr> &expr,
                                           PrettyStream &pretty) const {
    pretty.printComments(position());
    if (auto oper = dynamic_cast<ASTBinaryOperator *>(expr.get())) {
        if (operatorPrecedence(oper->operator_) < precedence) {
            pretty << "🤜";
            expr->toCode(pretty);
            pretty << "🤛";
            return;
        }
    }
    expr->toCode(pretty);
}

void ASTBinaryOperator::toCode(PrettyStream &pretty) const {
    pretty.printComments(position());
    auto precedence = operatorPrecedence(operator_);
    printBinaryOperand(precedence, left_, pretty);
    pretty << " " << utf8(operatorName(operator_)) << " ";
    printBinaryOperand(precedence, right_, pretty);
}

void ASTType::toCode(PrettyStream &pretty) const {
    if (optional_) {
        pretty << "🍬";
    }
    toCodeType(pretty);
}

void ASTTypeId::toCodeType(PrettyStream &pretty) const {
    if (!namespace_.empty()) {
        pretty << "🔶" << utf8(namespace_);
    }
    pretty << utf8(name_);
    if (!genericArgs_.empty()) {
        pretty << "🐚";
        for (auto &arg : genericArgs_) {
            pretty << arg;
        }
        pretty.refuseOffer() << "🍆";
    }
}

void ASTErrorType::toCodeType(PrettyStream &pretty) const {
    pretty << "🚨" << enum_ << content_;
}

void ASTCallableType::toCodeType(PrettyStream &pretty) const {
    pretty << "🍇";
    for (auto &type : params_) {
        pretty << type;
    }
    if (return_ != nullptr) {
        pretty << "➡️" << return_;
    }
    pretty << "🍉";
}

void ASTMultiProtocol::toCodeType(PrettyStream &pretty) const {
    pretty << "🍱";
    for (auto &type : protocols_) {
        pretty << type;
    }
    pretty << "🍱";
}

void ASTGenericVariable::toCodeType(PrettyStream &pretty) const {
    pretty << utf8(name_);
    pretty.offerSpace();
}

void ASTTypeValueType::toCodeType(PrettyStream &pretty) const {
    pretty << toString(tokenType_) << type_;
}

void ASTLiteralType::toCode(PrettyStream &pretty) const {
    pretty << type();
}

} // namespace EmojicodeCompiler
