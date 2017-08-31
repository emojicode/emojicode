//
//  ASTToCode.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 22/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../Parsing/OperatorHelper.hpp"
#include "../Types/Type.hpp"
#include "../AST/ASTBinaryOperator.hpp"
#include "../AST/ASTClosure.hpp"
#include "../AST/ASTControlFlow.hpp"
#include "../AST/ASTExpr.hpp"
#include "../AST/ASTInitialization.hpp"
#include "../AST/ASTLiterals.hpp"
#include "../AST/ASTMethod.hpp"
#include "../AST/ASTStatements.hpp"
#include "../AST/ASTTypeExpr.hpp"
#include "../AST/ASTUnary.hpp"
#include "../AST/ASTVariables.hpp"
#include "Prettyprinter.hpp"
#include <sstream>

namespace EmojicodeCompiler {

void ASTArguments::toCode(Prettyprinter &pretty) const {
    if (arguments_.empty()) {
        pretty << "❗️";
        return;
    }

    pretty << "❕";
    for (auto &arg : arguments_) {
        arg->toCode(pretty);
    }
    pretty.refuseOffer() << "❗️";
}

void ASTBlock::toCode(Prettyprinter &pretty) const {
    if (stmts_.empty()) {
        pretty << "🍇🍉\n";
        return;
    }
    pretty << "🍇\n";
    innerToCode(pretty);
    pretty.indent() << "🍉\n";
}

void ASTBlock::innerToCode(Prettyprinter &pretty) const {
    pretty.increaseIndent();
    for (auto &stmt : stmts_) {
        stmt->toCode(pretty);
        pretty << "\n";
    }
    pretty.decreaseIndent();
}

void ASTRepeatWhile::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🔁 ";
    condition_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTForIn::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🔂 " << utf8(varName_) << " ";
    iteratee_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTIf::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🍊 ";
    conditions_.front()->toCode(pretty);
    pretty << " ";
    blocks_.front().toCode(pretty);
    for (size_t i = 1; i < conditions_.size(); i++) {
        pretty.indent() << "🍋 ";
        conditions_[i]->toCode(pretty);
        pretty << " ";
        blocks_[i].toCode(pretty);
    }
    if (hasElse()) {
        pretty.indent() << "🍓 ";
        blocks_.back().toCode(pretty);
    }
}

void ASTClosure::toCode(Prettyprinter &pretty) const {
    pretty.printClosure(function_);
}

void ASTErrorHandler::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🥑 " << utf8(valueVarName_) << " ";
    value_->toCode(pretty);
    pretty << " ";
    valueBlock_.toCode(pretty);
    pretty.indent() << "🍓 " << utf8(errorVarName_) << " ";
    errorBlock_.toCode(pretty);
}

void ASTExprStatement::toCode(Prettyprinter &pretty) const {
    pretty.indent();
    expr_->toCode(pretty);
}

void ASTVariableDeclaration::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🍰 " << utf8(varName_) << " " << type_;
}

void ASTVariableAssignmentDecl::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🍮 " << utf8(varName_) << " ";
    expr_->toCode(pretty);
}

void ASTFrozenDeclaration::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🍦 " << utf8(varName_) << " ";
    expr_->toCode(pretty);
}

void ASTConditionalAssignment::toCode(Prettyprinter &pretty) const {
    pretty << "🍦 " << utf8(varName_) << " ";
    expr_->toCode(pretty);
}

void ASTGetVariable::toCode(Prettyprinter &pretty) const {
    pretty << utf8(name_);
}

void ASTSuperMethod::toCode(Prettyprinter &pretty) const {
    pretty << "🐿" << utf8(name_);
    args_.toCode(pretty);
}

void ASTTypeMethod::toCode(Prettyprinter &pretty) const {
    pretty << "🍩" << utf8(name_);
    callee_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTCaptureTypeMethod::toCode(Prettyprinter &pretty) const {
    pretty << "🌶🍩" << utf8(name_);
    callee_->toCode(pretty);
}

void ASTCaptureMethod::toCode(Prettyprinter &pretty) const {
    pretty << "🌶" << utf8(name_);
    callee_->toCode(pretty);
}

void ASTInitialization::toCode(Prettyprinter &pretty) const {
    pretty << "🔷";
    typeExpr_->toCode(pretty);
    pretty << utf8(name_);
    args_.toCode(pretty);
}

void ASTThisType::toCode(Prettyprinter &pretty) const {
    pretty << "🐕";
}

void ASTInferType::toCode(Prettyprinter &pretty) const {
    pretty << "⚫️";
}

void ASTStaticType::toCode(Prettyprinter &pretty) const {
    pretty << type_;
}

void ASTTypeFromExpr::toCode(Prettyprinter &pretty) const {
    pretty << "🔳";
    expr_->toCode(pretty);
}

void ASTMetaTypeInstantiation::toCode(Prettyprinter &pretty) const {
    pretty << "⬛️" << type_;
}

void ASTCallableCall::toCode(Prettyprinter &pretty) const {
    callable_->toCode(pretty);
    pretty << "⁉️";
    args_.toCode(pretty);
}

void ASTBooleanTrue::toCode(Prettyprinter &pretty) const {
    pretty << "👍";
}

void ASTBooleanFalse::toCode(Prettyprinter &pretty) const {
    pretty << "👎";
}

void ASTThis::toCode(Prettyprinter &pretty) const {
    pretty << "🐕";
}

void ASTIsNothigness::toCode(Prettyprinter &pretty) const {
    pretty << "☁️";
    value_->toCode(pretty);
}

void ASTIsError::toCode(Prettyprinter &pretty) const {
    pretty << "🚥";
    value_->toCode(pretty);
}

void ASTMetaTypeFromInstance::toCode(Prettyprinter &pretty) const {
    pretty << "⬜️";
    value_->toCode(pretty);
}

void ASTUnwrap::toCode(Prettyprinter &pretty) const {
    pretty << (error_ ? " 🚇" : " 🍺");
    value_->toCode(pretty);
}

void ASTNumberLiteral::toCode(Prettyprinter &pretty) const {
    pretty << utf8(string_);
    pretty.offerSpace();
}

void ASTSymbolLiteral::toCode(Prettyprinter &pretty) const {
    pretty << "🔟" << utf8(std::u32string(1, value_));
}

void ASTNothingness::toCode(Prettyprinter &pretty) const {
    pretty << "⚡️";
}

void ASTStringLiteral::toCode(Prettyprinter &pretty) const {
    pretty << "🔤" << utf8(value_) << "🔤";
}

void ASTRaise::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🚨";
    value_->toCode(pretty);
}

void ASTReturn::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🍎 ";
    if (value_ != nullptr) {
        value_->toCode(pretty);
    }
}

void ASTSuperinitializer::toCode(Prettyprinter &pretty) const {
    pretty.indent() << "🐐" << utf8(name_);
    arguments_.toCode(pretty);
}

void ASTCast::toCode(Prettyprinter &pretty) const {
    pretty << "🔲";
    value_->toCode(pretty);
    typeExpr_->toCode(pretty);
}

void ASTMethod::toCode(Prettyprinter &pretty) const {
    pretty << utf8(name_);
    callee_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTConcatenateLiteral::toCode(Prettyprinter &pretty) const {
    pretty << "🍪 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍪";
}

void ASTListLiteral::toCode(Prettyprinter &pretty) const {
    pretty << "🍨 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTDictionaryLiteral::toCode(Prettyprinter &pretty) const {
    pretty << "🍯 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTBinaryOperator::toCode(Prettyprinter &pretty) const {
    left_->toCode(pretty);
    pretty << " " << utf8(operatorName(operator_)) << " ";
    right_->toCode(pretty);
}

} // namespace EmojicodeCompiler
