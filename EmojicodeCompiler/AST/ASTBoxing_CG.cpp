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
    Type type = expr_->expressionType();
    type.unbox();
    type.setOptional(false);
    return fncg->getValuePtr(box, type);
}

Value* ASTBoxing::getSimpleOptional(Value *value, FnCodeGenerator *fncg) const {
    auto structType = fncg->typeHelper().llvmTypeFor(expressionType());
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = fncg->builder().CreateInsertValue(undef, value, 1);
    return fncg->builder().CreateInsertValue(simpleOptional, fncg->generator()->optionalValue(), 0);
}

Value* ASTBoxing::getSimpleOptionalWithoutValue(FnCodeGenerator *fncg) const {
    return fncg->getSimpleOptionalWithoutValue(expressionType());
}

void ASTBoxing::getPutValueIntoBox(Value *box, Value *value, FnCodeGenerator *fncg) const {
    auto metaType = fncg->generator()->valueTypeMetaFor(expr_->expressionType());
    fncg->builder().CreateStore(metaType, fncg->getMetaTypePtr(box));

    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }

    fncg->builder().CreateStore(value, getBoxValuePtr(box, fncg));
}

Value* ASTBoxing::getAllocaTheBox(FnCodeGenerator *fncg) const {
    auto box = fncg->builder().CreateAlloca(fncg->typeHelper().box());
    fncg->builder().CreateStore(expr_->generate(fncg), box);
    return box;
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
    return fncg->createIfElsePhi(hasNoValue, [this, fncg]() {
        return getSimpleOptionalWithoutValue(fncg);
    }, [this, box, fncg]() {
        return getSimpleOptional(getGetValueFromBox(box, fncg), fncg);
    });
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

    fncg->createIfElse(hasNoValue, [fncg, box]() {
        fncg->getMakeNoValue(box);
    }, [this, value, fncg, box]() {
        getPutValueIntoBox(box, fncg->builder().CreateExtractValue(value, 1), fncg);
    });
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
