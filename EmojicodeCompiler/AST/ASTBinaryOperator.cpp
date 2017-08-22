//
//  ASTBinaryOperator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBinaryOperator.hpp"

#include "../../EmojicodeInstructions.h"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Parsing/OperatorHelper.hpp"
#include "../Types/TypeExpectation.hpp"
#include "../Types/ValueType.hpp"
#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

Type ASTBinaryOperator::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type otype = left_->analyse(analyser, TypeExpectation());

    auto pair = builtInPrimitiveOperator(analyser, otype);
    if (pair.first) {
        builtIn_ = true;
        Type type = analyser->box(otype, TypeExpectation(false, false, false), &left_);
        analyser->expectType(type, &right_);
        if (pair.second.swap) {
            std::swap(left_, right_);
        }
        return pair.second.returnType;
    }

    Type type = analyser->box(otype, TypeExpectation(true, false), &left_);
    args_.addArguments(right_);

    return analyseMethodCall(analyser, operatorName(operator_), left_);
}

void ASTBinaryOperator::generateExpr(FnCodeGenerator *fncg) const {
    if (builtIn_) {
        left_->generate(fncg);
        right_->generate(fncg);
        fncg->wr().writeInstruction(instruction_);
        return;
    }

    CallCodeGenerator(fncg, instruction_).generate(*left_, args_, operatorName(operator_));
}

std::pair<bool, ASTBinaryOperator::BuiltIn> ASTBinaryOperator::builtInPrimitiveOperator(SemanticAnalyser *analyser,
                                                                                        const Type &type) {
    bool swap = false;
    if ((type.type() == TypeType::ValueType || type.type() == TypeType::Enum) &&
        type.valueType()->isPrimitive()) {
        if (type.valueType() == VT_DOUBLE) {
            switch (operator_) {
                case OperatorType::MultiplicationOperator:
                    instruction_ = INS_MULTIPLY_DOUBLE;
                    return std::make_pair(true, Type::doubl());
                case OperatorType::LessOperator:
                    swap = true;
                case OperatorType::GreaterOperator:
                    instruction_ = INS_GREATER_DOUBLE;
                    return std::make_pair(true, BuiltIn(Type::boolean(), swap));
                case OperatorType::LessOrEqualOperator:
                    swap = true;
                case OperatorType::GreaterOrEqualOperator:
                    instruction_ = INS_GREATER_OR_EQUAL_DOUBLE;
                    return std::make_pair(true, BuiltIn(Type::boolean(), swap));
                case OperatorType::DivisionOperator:
                    instruction_ = INS_DIVIDE_DOUBLE;
                    return std::make_pair(true, Type::doubl());
                case OperatorType::PlusOperator:
                    instruction_ = INS_ADD_DOUBLE;
                    return std::make_pair(true, Type::doubl());
                case OperatorType::MinusOperator:
                    instruction_ = INS_SUBTRACT_DOUBLE;
                    return std::make_pair(true, Type::doubl());
                case OperatorType::RemainderOperator:
                    instruction_ = INS_REMAINDER_DOUBLE;
                    return std::make_pair(true, Type::doubl());
                case OperatorType::EqualOperator:
                    instruction_ = INS_EQUAL_DOUBLE;
                    return std::make_pair(true, Type::boolean());
                default:
                    break;
            }
        }
        else if (type.valueType() == VT_INTEGER) {
            switch (operator_) {
                case OperatorType::MultiplicationOperator:
                    instruction_ = INS_MULTIPLY_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::BitwiseAndOperator:
                    instruction_ = INS_BINARY_AND_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::BitwiseOrOperator:
                    instruction_ = INS_BINARY_OR_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::BitwiseXorOperator:
                    instruction_ = INS_BINARY_XOR_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::LessOperator:
                    swap = true;
                case OperatorType::GreaterOperator:
                    instruction_ = INS_GREATER_INTEGER;
                    return std::make_pair(true, BuiltIn(Type::boolean(), swap));
                case OperatorType::LessOrEqualOperator:
                    swap = true;
                case OperatorType::GreaterOrEqualOperator:
                    instruction_ = INS_GREATER_OR_EQUAL_INTEGER;
                    return std::make_pair(true, BuiltIn(Type::boolean(), swap));
                case OperatorType::ShiftLeftOperator:
                    instruction_ = INS_SHIFT_LEFT_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::ShiftRightOperator:
                    instruction_ = INS_SHIFT_RIGHT_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::DivisionOperator:
                    instruction_ = INS_DIVIDE_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::PlusOperator:
                    instruction_ = INS_ADD_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::MinusOperator:
                    instruction_ = INS_SUBTRACT_INTEGER;
                    return std::make_pair(true, Type::integer());
                case OperatorType::RemainderOperator:
                    instruction_ = INS_REMAINDER_INTEGER;
                    return std::make_pair(true, Type::integer());
                default:
                    break;
            }
        }
        else if (type.valueType() == VT_BOOLEAN) {
            switch (operator_) {
                case OperatorType::LogicalAndOperator:
                    instruction_ = INS_AND_BOOLEAN;
                    return std::make_pair(true, Type::boolean());
                case OperatorType::LogicalOrOperator:
                    instruction_ = INS_OR_BOOLEAN;
                    return std::make_pair(true, Type::boolean());
                default:
                    break;
            }
        }

        if (operator_ == OperatorType::IdentityOperator) {
            if (!type.compatibleTo(Type::someobject(), analyser->typeContext())) {
                throw CompilerError(position(), "The identity operator can only be used with objects.");
            }
            instruction_ = INS_SAME_OBJECT;
            return std::make_pair(true, Type::boolean());
        }

        if (operator_ == OperatorType::EqualOperator) {
            instruction_ = INS_EQUAL_PRIMITIVE;
            return std::make_pair(true, Type::boolean());
        }
    }

    return std::make_pair(false, Type::nothingness());
}

}  // namespace EmojicodeCompiler
