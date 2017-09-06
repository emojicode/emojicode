//
//  ASTBinaryOperator_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBinaryOperator.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTBinaryOperator::generateExpr(FnCodeGenerator *fncg) const {
    if (builtIn_ != BuiltInType::None) {
        auto left = left_->generate(fncg);
        auto right = right_->generate(fncg);
        switch (builtIn_) {
            case BuiltInType::DoubleDivide:
                return fncg->builder().CreateFDiv(left, right);
            case BuiltInType::DoubleRemainder:
                return fncg->builder().CreateFRem(left, right);
            case BuiltInType::DoubleSubstract:
                return fncg->builder().CreateFSub(left, right);
            case BuiltInType::DoubleAdd:
                return fncg->builder().CreateFAdd(left, right);
            case BuiltInType::DoubleMultiply:
                return fncg->builder().CreateFMul(left, right);
            case BuiltInType::DoubleLessOrEqual:
                return fncg->builder().CreateFCmpULE(left, right);
            case BuiltInType::DoubleLess:
                return fncg->builder().CreateFCmpULT(left, right);
            case BuiltInType::DoubleGreaterOrEqual:
                return fncg->builder().CreateFCmpUGE(left, right);
            case BuiltInType::DoubleGreater:
                return fncg->builder().CreateFCmpUGT(left, right);
            case BuiltInType::DoubleEqual:
                return fncg->builder().CreateFCmpUEQ(left, right);
            case BuiltInType::IntegerAdd:
                return fncg->builder().CreateAdd(left, right);
            case BuiltInType::IntegerMultiply:
                return fncg->builder().CreateMul(left, right);
            case BuiltInType::IntegerDivide:
                return fncg->builder().CreateSDiv(left, right);
            case BuiltInType::IntegerSubstract:
                return fncg->builder().CreateSub(left, right);
            case BuiltInType::IntegerLess:
                return fncg->builder().CreateICmpSLT(left, right);
            case BuiltInType::IntegerLessOrEqual:
                return fncg->builder().CreateICmpSLE(left, right);
            case BuiltInType::IntegerGreater:
                return fncg->builder().CreateICmpSGT(left, right);
            case BuiltInType::IntegerGreaterOrEqual:
                return fncg->builder().CreateICmpSGE(left, right);
            case BuiltInType::IntegerRightShift:
                return fncg->builder().CreateLShr(left, right);
            case BuiltInType::IntegerLeftShift:
                return fncg->builder().CreateShl(left, right);
            case BuiltInType::IntegerOr:
                return fncg->builder().CreateOr(left, right);
            case BuiltInType::IntegerXor:
                return fncg->builder().CreateXor(left, right);
            case BuiltInType::IntegerRemainder:
                return fncg->builder().CreateSRem(left, right);
            case BuiltInType::IntegerAnd:
                return fncg->builder().CreateAnd(left, right);
            case BuiltInType::BooleanOr:
                return fncg->builder().CreateOr(left, right);
            case BuiltInType::BooleanAnd:
                return fncg->builder().CreateAnd(left, right);
            case BuiltInType::Equal:
                return fncg->builder().CreateICmpEQ(left, right);
            default:
                break;
        }
    }

    return CallCodeGenerator(fncg, callType_).generate(left_->generate(fncg), calleeType_, args_,
                                                       operatorName(operator_));
}

}  // namespace EmojicodeCompiler
