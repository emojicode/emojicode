//
//  ASTToCode.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 22/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTToCode.hpp"
#include "ASTExpr.hpp"
#include "ASTTypeExpr.hpp"
#include "ASTStatements.hpp"
#include "ASTControlFlow.hpp"
#include "ASTLiterals.hpp"
#include "ASTUnary.hpp"
#include "ASTVariables.hpp"
#include "ASTMethod.hpp"
#include "ASTBinaryOperator.hpp"
#include "ASTInitialization.hpp"
#include "ASTClosure.hpp"
#include "../Types/Type.hpp"
#include "../Parsing/OperatorHelper.hpp"
#include <sstream>

namespace EmojicodeCompiler {

std::ostream& operator<<(std::ostream &stream, const Type &type) {
    stream << type.toString(Type::nothingness(), false);
    return stream;
}

std::stringstream& indent(std::stringstream &stream, unsigned int indentation) {
    stream << std::string(indentation * 2, ' ');
    return stream;
}

void ASTArguments::toCode(std::stringstream &stream) const {
    if (arguments_.empty()) {
        stream << "❗️";
        return;
    }

    stream << "❕";
    for (auto &arg : arguments_) {
        arg->toCode(stream);
    }
    stream << "❗️";
}

void ASTBlock::toCode(std::stringstream &stream, unsigned int indentation) const {
    stream << "🍇\n";
    for (auto &stmt : stmts_) {
        stmt->toCode(stream, indentation + 1);
        stream << "\n";
    }
    indent(stream, indentation) << "🍉\n";
}

void ASTRepeatWhile::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🔁 ";
    condition_->toCode(stream);
    block_.toCode(stream, indentation);
}

void ASTForIn::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🔂 " << utf8(varName_) << " ";
    iteratee_->toCode(stream);
    block_.toCode(stream, indentation);
}

void ASTIf::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🍊 ";
    conditions_.front()->toCode(stream);
    stream << " ";
    blocks_.front().toCode(stream, indentation);
    for (size_t i = 1; i < conditions_.size(); i++) {
        indent(stream, indentation) << "🍋 ";
        conditions_[i]->toCode(stream);
        stream << " ";
        blocks_[i].toCode(stream, indentation);
    }
    if (hasElse()) {
        indent(stream, indentation) << "🍓 ";
        blocks_.back().toCode(stream, indentation);
    }
}

void ASTClosure::toCode(std::stringstream &stream) const {
    stream << "🍇";
    stream << "👵 Closures cannot be generated right now 👵";
    stream << "🍉";
}

void ASTErrorHandler::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🥑 " << utf8(valueVarName_) << " ";
    value_->toCode(stream);
    stream << " ";
    valueBlock_.toCode(stream, indentation);
    indent(stream, indentation) << "🍓 " << utf8(errorVarName_) << " ";
    errorBlock_.toCode(stream, indentation);
}

void ASTExprStatement::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation);
    expr_->toCode(stream);
}

void ASTVariableDeclaration::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🍰 " << utf8(varName_) << " " << type_;
}

void ASTVariableAssignmentDecl::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🍮 " << utf8(varName_) << " ";
    expr_->toCode(stream);
}

void ASTFrozenDeclaration::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🍦 " << utf8(varName_) << " ";
    expr_->toCode(stream);
}

void ASTConditionalAssignment::toCode(std::stringstream &stream) const {
    stream << "🍦 " << utf8(varName_) << " ";
    expr_->toCode(stream);
}

void ASTGetVariable::toCode(std::stringstream &stream) const {
    stream << utf8(name_);
}

void ASTSuperMethod::toCode(std::stringstream &stream) const {
    stream << "🐿" << utf8(name_);
    args_.toCode(stream);
}

void ASTTypeMethod::toCode(std::stringstream &stream) const {
    stream << "🍩" << utf8(name_);
    callee_->toCode(stream);
    args_.toCode(stream);
}

void ASTCaptureTypeMethod::toCode(std::stringstream &stream) const {
    stream << "🌶🍩" << utf8(name_);
    callee_->toCode(stream);
}

void ASTCaptureMethod::toCode(std::stringstream &stream) const {
    stream << "🌶" << utf8(name_);
    callee_->toCode(stream);
}

void ASTInitialization::toCode(std::stringstream &stream) const {
    stream << "🔷" << utf8(name_);
    typeExpr_->toCode(stream);
    args_.toCode(stream);
}

void ASTThisType::toCode(std::stringstream &stream) const {
    stream << "🐕";
}

void ASTInferType::toCode(std::stringstream &stream) const {
    stream << "⚫️";
}

void ASTStaticType::toCode(std::stringstream &stream) const {
    stream << type_;
}

void ASTTypeFromExpr::toCode(std::stringstream &stream) const {
    stream << "🔳";
    expr_->toCode(stream);
}

void ASTMetaTypeInstantiation::toCode(std::stringstream &stream) const {
    stream << "⬛️" << type_;
}

void ASTCallableCall::toCode(std::stringstream &stream) const {
    callable_->toCode(stream);
    stream << "⁉️";
    args_.toCode(stream);
}

void ASTBooleanTrue::toCode(std::stringstream &stream) const {
    stream << "👍";
}

void ASTBooleanFalse::toCode(std::stringstream &stream) const {
    stream << "👎";
}

void ASTThis::toCode(std::stringstream &stream) const {
    stream << "🐕";
}

void ASTIsNothigness::toCode(std::stringstream &stream) const {
    stream << "☁️";
    value_->toCode(stream);
}

void ASTIsError::toCode(std::stringstream &stream) const {
    stream << "🚥";
    value_->toCode(stream);
}

void ASTMetaTypeFromInstance::toCode(std::stringstream &stream) const {
    stream << "⬜️";
    value_->toCode(stream);
}

void ASTUnwrap::toCode(std::stringstream &stream) const {
    stream << (error_ ? " 🚇" : " 🍺");
    value_->toCode(stream);
}

void ASTNumberLiteral::toCode(std::stringstream &stream) const {
    switch (type_) {
        case ASTNumberLiteral::NumberType::Integer:
            stream << integerValue_;
            break;
        case ASTNumberLiteral::NumberType::Double:
            stream << doubleValue_;
            break;
    }
}

void ASTSymbolLiteral::toCode(std::stringstream &stream) const {
    stream << "🔟" << utf8(std::u32string(1, value_));
}

void ASTNothingness::toCode(std::stringstream &stream) const {
    stream << "⚡️";
}

void ASTStringLiteral::toCode(std::stringstream &stream) const {
    stream << "🔤" << utf8(value_) << "🔤";
}

void ASTRaise::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🚨";
    value_->toCode(stream);
}

void ASTReturn::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🍎";
    value_->toCode(stream);
}

void ASTSuperinitializer::toCode(std::stringstream &stream, unsigned int indentation) const {
    indent(stream, indentation) << "🐐" << utf8(name_);
    arguments_.toCode(stream);
}

void ASTCast::toCode(std::stringstream &stream) const {
    stream << "🔲";
    value_->toCode(stream);
    typeExpr_->toCode(stream);
}

void ASTMethod::toCode(std::stringstream &stream) const {
    stream << utf8(name_);
    callee_->toCode(stream);
    args_.toCode(stream);
}

void ASTConcatenateLiteral::toCode(std::stringstream &stream) const {
    stream << "🍪 ";
    for (auto &val : values_) {
        val->toCode(stream);
        stream << " ";
    }
    stream << "🍪";
}

void ASTListLiteral::toCode(std::stringstream &stream) const {
    stream << "🍨 ";
    for (auto &val : values_) {
        val->toCode(stream);
        stream << " ";
    }
    stream << "🍆";
}

void ASTDictionaryLiteral::toCode(std::stringstream &stream) const {
    stream << "🍯 ";
    for (auto &val : values_) {
        val->toCode(stream);
        stream << " ";
    }
    stream << "🍆";
}

void ASTBinaryOperator::toCode(std::stringstream &stream) const {
    left_->toCode(stream);
    stream << " " << utf8(operatorName(operator_)) << " ";
    right_->toCode(stream);
}

}
