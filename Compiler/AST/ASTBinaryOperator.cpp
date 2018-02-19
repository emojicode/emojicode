//
//  ASTBinaryOperator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBinaryOperator.hpp"
#include "Compiler.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Types/ValueType.hpp"
#include "ASTLiterals.hpp"

namespace EmojicodeCompiler {

Type ASTBinaryOperator::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (operator_ == OperatorType::EqualOperator) {
        if (std::dynamic_pointer_cast<ASTNoValue>(right_) != nullptr) {
            return analyseIsNoValue(analyser, left_, BuiltInType::IsNoValueLeft);
        }
        if (std::dynamic_pointer_cast<ASTNoValue>(left_) != nullptr) {
            return analyseIsNoValue(analyser, right_, BuiltInType::IsNoValueRight);
        }
    }

    Type otype = left_->analyse(analyser, TypeExpectation());

    auto pair = builtInPrimitiveOperator(analyser, otype);
    if (pair.first) {
        Type type = analyser->comply(otype, TypeExpectation(false, false, false), &left_);
        analyser->expectType(type, &right_);
        return pair.second.returnType;
    }

    Type type = analyser->comply(otype, TypeExpectation(true, false), &left_);
    args_.addArguments(right_);
    return analyseMethodCall(analyser, operatorName(operator_), left_);
}

Type ASTBinaryOperator::analyseIsNoValue(SemanticAnalyser *analyser, std::shared_ptr<ASTExpr> &expr,
                                         BuiltInType builtInType) {
    Type type = analyser->expect(TypeExpectation(false, false), &expr);
    if (!type.optional() && type.type() != TypeType::Something) {
        throw CompilerError(position(), "Only optionals and ⚪️ can be compared to 🤷‍♀️.");
    }
    builtIn_ = builtInType;
    return analyser->boolean();
}

std::pair<bool, ASTBinaryOperator::BuiltIn> ASTBinaryOperator::builtInPrimitiveOperator(SemanticAnalyser *analyser,
                                                                                        const Type &type) {
    if ((type.type() == TypeType::ValueType || type.type() == TypeType::Enum) &&
        type.valueType()->isPrimitive()) {
        if (type.valueType() == analyser->compiler()->sReal) {
            switch (operator_) {
                case OperatorType::MultiplicationOperator:
                    builtIn_ = BuiltInType::DoubleMultiply;
                    return std::make_pair(true, BuiltIn(analyser->doubleType()));
                case OperatorType::LessOperator:
                    builtIn_ = BuiltInType::DoubleLess;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::GreaterOperator:
                    builtIn_ = BuiltInType::DoubleGreater;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::LessOrEqualOperator:
                    builtIn_ = BuiltInType::DoubleLessOrEqual;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::GreaterOrEqualOperator:
                    builtIn_ = BuiltInType::DoubleGreaterOrEqual;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::DivisionOperator:
                    builtIn_ = BuiltInType::DoubleDivide;
                    return std::make_pair(true, BuiltIn(analyser->doubleType()));
                case OperatorType::PlusOperator:
                    builtIn_ = BuiltInType::DoubleAdd;
                    return std::make_pair(true, BuiltIn(analyser->doubleType()));
                case OperatorType::MinusOperator:
                    builtIn_ = BuiltInType::DoubleSubstract;
                    return std::make_pair(true, BuiltIn(analyser->doubleType()));
                case OperatorType::RemainderOperator:
                    builtIn_ = BuiltInType::DoubleRemainder;
                    return std::make_pair(true, BuiltIn(analyser->doubleType()));
                case OperatorType::EqualOperator:
                    builtIn_ = BuiltInType::DoubleEqual;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                default:
                    break;
            }
        }
        else if (type.valueType() == analyser->compiler()->sInteger) {
            switch (operator_) {
                case OperatorType::MultiplicationOperator:
                    builtIn_ = BuiltInType::IntegerMultiply;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::BitwiseAndOperator:
                    builtIn_ = BuiltInType::IntegerAnd;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::BitwiseOrOperator:
                    builtIn_ = BuiltInType::IntegerOr;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::BitwiseXorOperator:
                    builtIn_ = BuiltInType::IntegerXor;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::LessOperator:
                    builtIn_ = BuiltInType::IntegerLess;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::GreaterOperator:
                    builtIn_ = BuiltInType::IntegerGreater;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::LessOrEqualOperator:
                    builtIn_ = BuiltInType::IntegerLessOrEqual;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::GreaterOrEqualOperator:
                    builtIn_ = BuiltInType::IntegerGreaterOrEqual;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::ShiftLeftOperator:
                    builtIn_ = BuiltInType::IntegerLeftShift;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::ShiftRightOperator:
                    builtIn_ = BuiltInType::IntegerRightShift;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::DivisionOperator:
                    builtIn_ = BuiltInType::IntegerDivide;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::PlusOperator:
                    builtIn_ = BuiltInType::IntegerAdd;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::MinusOperator:
                    builtIn_ = BuiltInType::IntegerSubstract;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                case OperatorType::RemainderOperator:
                    builtIn_ = BuiltInType::IntegerRemainder;
                    return std::make_pair(true, BuiltIn(analyser->integer()));
                default:
                    break;
            }
        }
        else if (type.valueType() == analyser->compiler()->sBoolean) {
            switch (operator_) {
                case OperatorType::LogicalAndOperator:
                    builtIn_ = BuiltInType::BooleanAnd;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                case OperatorType::LogicalOrOperator:
                    builtIn_ = BuiltInType::BooleanOr;
                    return std::make_pair(true, BuiltIn(analyser->boolean()));
                default:
                    break;
            }
        }

        if (operator_ == OperatorType::IdentityOperator) {
            if (!type.compatibleTo(Type::someobject(), analyser->typeContext())) {
                throw CompilerError(position(), "The identity operator can only be used with objects.");
            }
            builtIn_ = BuiltInType::Equal;
            return std::make_pair(true, BuiltIn(analyser->boolean()));
        }

        if (operator_ == OperatorType::EqualOperator) {
            builtIn_ = BuiltInType::Equal;
            return std::make_pair(true, BuiltIn(analyser->boolean()));
        }
    }

    return std::make_pair(false, BuiltIn(Type::noReturn()));
}

}  // namespace EmojicodeCompiler
