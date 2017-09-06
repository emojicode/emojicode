//
//  ASTBoxing_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBoxing.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Functions/BoxingLayer.hpp"

namespace EmojicodeCompiler {

Value* ASTBoxing::getBoxValuePtr(Value *box, FnCodeGenerator *fncg) const {
    std::vector<Value *> idx2{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 1),
    };
    auto type = fncg->typeHelper().llvmTypeFor(expr_->expressionType())->getPointerTo();
    return fncg->builder().CreateBitCast(fncg->builder().CreateGEP(box, idx2), type);
}

Value* ASTBoxing::getSimpleOptional(Value *value, FnCodeGenerator *fncg) const {
    auto structType = fncg->typeHelper().llvmTypeFor(expressionType());
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = fncg->builder().CreateInsertValue(undef, value, 1);
    return fncg->builder().CreateInsertValue(simpleOptional, fncg->generator()->optionalValue(), 0);
}

Value* ASTBoxing::getSimpleOptionalWithoutValue(FnCodeGenerator *fncg) const {
    auto structType = fncg->typeHelper().llvmTypeFor(expressionType());
    auto undef = llvm::UndefValue::get(structType);
    return fncg->builder().CreateInsertValue(undef, fncg->generator()->optionalNoValue(), 0);
}

void ASTBoxing::getPutValueIntoBox(Value *box, Value *value, FnCodeGenerator *fncg) const {
    auto metaType = llvm::Constant::getNullValue(fncg->typeHelper().valueTypeMetaTypePtr());
    fncg->builder().CreateStore(metaType, fncg->getMetaTypePtr(box));

    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }

    fncg->builder().CreateStore(value, getBoxValuePtr(box, fncg));
}

Value* ASTBoxing::getAllocaTheBox(FnCodeGenerator *fncg) const {
    auto box = fncg->builder().CreateAlloca(fncg->typeHelper().box());
    return fncg->builder().CreateStore(expr_->generate(fncg), box);
}

Value* ASTBoxing::getGetValueFromBox(Value *box, FnCodeGenerator *fncg) const {
    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }
    return fncg->builder().CreateLoad(getBoxValuePtr(box, fncg));
}

Value* ASTBoxToSimple::generateExpr(FnCodeGenerator *fncg) const {
    return getGetValueFromBox(getAllocaTheBox(fncg), fncg);
}

Value* ASTBoxToSimpleOptional::generateExpr(FnCodeGenerator *fncg) const {
    auto box = getAllocaTheBox(fncg);

    auto hasNoValue = fncg->getHasBoxNoValue(box);

    auto function = fncg->builder().GetInsertBlock()->getParent();
    auto noValueBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "noValue", function);
    auto valueBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "value", function);
    auto mergeBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "cont", function);

    fncg->builder().CreateCondBr(hasNoValue, noValueBlock, valueBlock);

    fncg->builder().SetInsertPoint(noValueBlock);
    auto noValueValue = getSimpleOptionalWithoutValue(fncg);
    fncg->builder().CreateBr(mergeBlock);

    fncg->builder().SetInsertPoint(valueBlock);
    auto value = getSimpleOptional(getGetValueFromBox(box, fncg), fncg);
    fncg->builder().CreateBr(mergeBlock);

    fncg->builder().SetInsertPoint(mergeBlock);
    auto phi = fncg->builder().CreatePHI(fncg->typeHelper().llvmTypeFor(expressionType()), 2);
    phi->addIncoming(noValueValue, noValueBlock);
    phi->addIncoming(value, valueBlock);
    return phi;
}

Value* ASTSimpleToSimpleOptional::generateExpr(FnCodeGenerator *fncg) const {
    return getSimpleOptional(expr_->generate(fncg), fncg);
}

Value* ASTSimpleToBox::generateExpr(FnCodeGenerator *fncg) const {
    auto box = fncg->builder().CreateAlloca(fncg->typeHelper().box());
    getPutValueIntoBox(box, expr_->generate(fncg), fncg);
    return fncg->builder().CreateLoad(box);
}

Value* ASTSimpleOptionalToBox::generateExpr(FnCodeGenerator *fncg) const {
    auto value = expr_->generate(fncg);
    auto box = fncg->builder().CreateAlloca(fncg->typeHelper().box());

    auto hasNoValue = fncg->getHasNoValue(value);

    auto function = fncg->builder().GetInsertBlock()->getParent();
    auto noValueBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "noValue", function);
    auto valueBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "value", function);
    auto mergeBlock = llvm::BasicBlock::Create(fncg->generator()->context(), "cont", function);

    fncg->builder().CreateCondBr(hasNoValue, noValueBlock, valueBlock);

    fncg->builder().SetInsertPoint(noValueBlock);
    auto metaType = llvm::Constant::getNullValue(fncg->typeHelper().valueTypeMetaTypePtr());
    fncg->builder().CreateStore(metaType, fncg->getMetaTypePtr(box));
    fncg->builder().CreateBr(mergeBlock);

    fncg->builder().SetInsertPoint(valueBlock);
    getPutValueIntoBox(box, value, fncg);
    fncg->builder().CreateBr(mergeBlock);
    fncg->builder().SetInsertPoint(mergeBlock);
    return fncg->builder().CreateLoad(box);
}

Value* ASTDereference::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->builder().CreateLoad(expr_->generate(fncg));
}

Value* ASTCallableBox::generateExpr(FnCodeGenerator *fncg) const {
//    expr_->generate(fncg);
//    fncg->wr().writeInstruction(INS_CLOSURE_BOX);
//    fncg->wr().writeInstruction(boxingLayer_->vtiForUse());
    return nullptr;
}

Value* ASTStoreTemporarily::generateExpr(FnCodeGenerator *fncg) const {
//    auto rtype = expr_->expressionType();
//    auto &variable = fncg->scoper().declareVariable(varId_, rtype);
//    expr_->generate(fncg);
//    fncg->copyToVariable(variable.stackIndex, false, rtype);
//    variable.initialize(fncg->wr().count());
//    fncg->pushVariableReference(variable.stackIndex, false);
    return nullptr;
}

}  // namespace EmojicodeCompiler
