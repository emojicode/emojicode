//
//  ASTExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Types/Class.hpp"
#include "ASTProxyExpr.hpp"
#include "ASTTypeExpr.hpp"

namespace EmojicodeCompiler {

Value* ASTExpr::generate(FnCodeGenerator *fncg) const {
    return generateExpr(fncg);
}

Value* ASTMetaTypeInstantiation::generateExpr(FnCodeGenerator *fncg) const {
    return type_.eclass()->classMeta();
}

Value* ASTCast::generateExpr(FnCodeGenerator *fncg) const {
//    typeExpr_->generate(fncg);
//    value_->generate(fncg);
//    switch (castType_) {
//        case CastType::ClassDowncast:
//            fncg->wr().writeInstruction(INS_DOWNCAST_TO_CLASS);
//            break;
//        case CastType::ToClass:
//            fncg->wr().writeInstruction(INS_CAST_TO_CLASS);
//            break;
//        case CastType::ToValueType:
//            fncg->wr().writeInstruction(INS_CAST_TO_VALUE_TYPE);
//            fncg->wr().writeInstruction(typeExpr_->expressionType().boxIdentifier());
//            break;
//        case CastType::ToProtocol:
//            fncg->wr().writeInstruction(INS_CAST_TO_PROTOCOL);
//            fncg->wr().writeInstruction(typeExpr_->expressionType().protocol()->index);
//            break;
//    }
    return nullptr;
}

Value* ASTConditionalAssignment::generateExpr(FnCodeGenerator *fncg) const {
    auto optional = expr_->generate(fncg);

    auto value = fncg->builder().CreateExtractValue(optional, std::vector<unsigned int>{1}, "condValue");
    fncg->scoper().getVariable(varId_) = LocalVariable(false, value);

    auto flag = fncg->builder().CreateExtractValue(optional, std::vector<unsigned int>{0});
    auto constant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(fncg->generator()->context()), 1);
    return fncg->builder().CreateICmpEQ(flag, constant);
}

Value* ASTTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
    auto ctype = valueType_ ? CallType::StaticContextfreeDispatch : CallType::DynamicDispatch;
    return TypeMethodCallCodeGenerator(fncg, ctype).generate(callee_->generate(fncg), callee_->expressionType(),
                                                             args_, name_);
}

Value* ASTSuperMethod::generateExpr(FnCodeGenerator *fncg) const {
    auto castedThis = fncg->builder().CreateBitCast(fncg->thisValue(), fncg->typeHelper().llvmTypeFor(calleeType_));
    return CallCodeGenerator(fncg, CallType::StaticDispatch).generate(castedThis, calleeType_, args_, name_);
}

Value* ASTCallableCall::generateExpr(FnCodeGenerator *fncg) const {
    return CallableCallCodeGenerator(fncg).generate(callable_->generate(fncg), callable_->expressionType(),
                                                    args_, std::u32string());
}

Value* ASTCaptureMethod::generateExpr(FnCodeGenerator *fncg) const {
//    callee_->generate(fncg);
//    fncg->wr().writeInstruction(INS_CAPTURE_METHOD);
//    fncg->wr().writeInstruction(callee_->expressionType().eclass()->lookupMethod(name_)->vtiForUse());
    return nullptr;
}

Value* ASTCaptureTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
//    if (contextedFunction_) {
//        fncg->wr().writeInstruction(INS_CAPTURE_CONTEXTED_FUNCTION);
//    }
//    else {
//        callee_->generate(fncg);
//        fncg->wr().writeInstruction(INS_CAPTURE_TYPE_METHOD);
//    }
//    fncg->wr().writeInstruction(callee_->expressionType().typeDefinition()->lookupTypeMethod(name_)->vtiForUse());
    return nullptr;
}

}  // namespace EmojicodeCompiler
