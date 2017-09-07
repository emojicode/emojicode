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
    if (castType_ == CastType::ClassDowncast) {
        return downcast(fncg);
    }

    auto box = fncg->builder().CreateAlloca(fncg->typeHelper().box());
    fncg->builder().CreateStore(value_->generate(fncg), box);
    Value *is = nullptr;
    switch (castType_) {
        case CastType::ToClass:
            is = castToClass(fncg, box);
            break;
        case CastType::ToValueType:
            is = castToValueType(fncg, box);
            break;
        case CastType::ToProtocol:
            // TODO: implement
            return nullptr;
        case CastType::ClassDowncast:
            break;
    }

    fncg->createIfElse(is, []() {}, [fncg, box]() {
        fncg->getMakeNoValue(box);
    });
    return fncg->builder().CreateLoad(box);
}

Value* ASTCast::downcast(FnCodeGenerator *fncg) const {
    auto value = value_->generate(fncg);
    auto meta = fncg->getMetaFromObject(value);
    auto toType = typeExpr_->expressionType();
    return fncg->createIfElsePhi(fncg->builder().CreateICmpEQ(meta, typeExpr_->generate(fncg)), [toType, fncg, value]() {
        auto casted = fncg->builder().CreateBitCast(value, fncg->typeHelper().llvmTypeFor(toType));
        return fncg->getSimpleOptionalWithValue(casted, toType.setOptional());
    }, [fncg, toType]() {
        return fncg->getSimpleOptionalWithoutValue(toType.setOptional());
    });
}

Value* ASTCast::castToValueType(FnCodeGenerator *fncg, Value *box) const {
    auto meta = fncg->getMetaTypePtr(box);
    return fncg->builder().CreateICmpEQ(fncg->builder().CreateLoad(meta), typeExpr_->generate(fncg));
}

Value* ASTCast::castToClass(FnCodeGenerator *fncg, Value *box) const {
    auto meta = fncg->getMetaTypePtr(box);
    auto toType = typeExpr_->expressionType();
    auto expMeta = fncg->generator()->valueTypeMetaFor(toType);
    auto ptr = fncg->builder().CreateLoad(fncg->getValuePtr(box, typeExpr_->expressionType()));
    auto obj = fncg->builder().CreateBitCast(ptr, fncg->typeHelper().llvmTypeFor(toType));
    auto klassPtr = fncg->getObjectMetaPtr(obj);

    auto isClass = fncg->builder().CreateICmpEQ(fncg->builder().CreateLoad(meta), expMeta);
    auto isCorrectClass = fncg->builder().CreateICmpEQ(typeExpr_->generate(fncg), fncg->builder().CreateLoad(klassPtr));
    return fncg->builder().CreateMul(isClass, isCorrectClass);
}

Value* ASTConditionalAssignment::generateExpr(FnCodeGenerator *fncg) const {
    auto optional = expr_->generate(fncg);

    auto value = fncg->builder().CreateExtractValue(optional, 1, "condValue");
    fncg->scoper().getVariable(varId_) = LocalVariable(false, value);

    auto flag = fncg->builder().CreateExtractValue(optional, 0);
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
