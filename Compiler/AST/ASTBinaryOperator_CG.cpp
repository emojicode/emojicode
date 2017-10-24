//
//  ASTBinaryOperator_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBinaryOperator.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTBinaryOperator::generate(FunctionCodeGenerator *fg) const {
    if (builtIn_ != BuiltInType::None) {
        auto left = left_->generate(fg);
        auto right = right_->generate(fg);
        switch (builtIn_) {
            case BuiltInType::DoubleDivide:
                return fg->builder().CreateFDiv(left, right);
            case BuiltInType::DoubleRemainder:
                return fg->builder().CreateFRem(left, right);
            case BuiltInType::DoubleSubstract:
                return fg->builder().CreateFSub(left, right);
            case BuiltInType::DoubleAdd:
                return fg->builder().CreateFAdd(left, right);
            case BuiltInType::DoubleMultiply:
                return fg->builder().CreateFMul(left, right);
            case BuiltInType::DoubleLessOrEqual:
                return fg->builder().CreateFCmpULE(left, right);
            case BuiltInType::DoubleLess:
                return fg->builder().CreateFCmpULT(left, right);
            case BuiltInType::DoubleGreaterOrEqual:
                return fg->builder().CreateFCmpUGE(left, right);
            case BuiltInType::DoubleGreater:
                return fg->builder().CreateFCmpUGT(left, right);
            case BuiltInType::DoubleEqual:
                return fg->builder().CreateFCmpUEQ(left, right);
            case BuiltInType::IntegerAdd:
                return fg->builder().CreateAdd(left, right);
            case BuiltInType::IntegerMultiply:
                return fg->builder().CreateMul(left, right);
            case BuiltInType::IntegerDivide:
                return fg->builder().CreateSDiv(left, right);
            case BuiltInType::IntegerSubstract:
                return fg->builder().CreateSub(left, right);
            case BuiltInType::IntegerLess:
                return fg->builder().CreateICmpSLT(left, right);
            case BuiltInType::IntegerLessOrEqual:
                return fg->builder().CreateICmpSLE(left, right);
            case BuiltInType::IntegerGreater:
                return fg->builder().CreateICmpSGT(left, right);
            case BuiltInType::IntegerGreaterOrEqual:
                return fg->builder().CreateICmpSGE(left, right);
            case BuiltInType::IntegerRightShift:
                return fg->builder().CreateLShr(left, right);
            case BuiltInType::IntegerLeftShift:
                return fg->builder().CreateShl(left, right);
            case BuiltInType::IntegerOr:
                return fg->builder().CreateOr(left, right);
            case BuiltInType::IntegerXor:
                return fg->builder().CreateXor(left, right);
            case BuiltInType::IntegerRemainder:
                return fg->builder().CreateSRem(left, right);
            case BuiltInType::IntegerAnd:
                return fg->builder().CreateAnd(left, right);
            case BuiltInType::BooleanOr:
                return fg->builder().CreateOr(left, right);
            case BuiltInType::BooleanAnd:
                return fg->builder().CreateAnd(left, right);
            case BuiltInType::Equal:
                return fg->builder().CreateICmpEQ(left, right);
            default:
                break;
        }
    }

    return CallCodeGenerator(fg, callType_).generate(left_->generate(fg), calleeType_, args_,
                                                       operatorName(operator_));
}

}  // namespace EmojicodeCompiler
