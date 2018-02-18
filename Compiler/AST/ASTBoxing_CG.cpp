//
//  ASTBoxing_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBoxing.hpp"
#include "ASTInitialization.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

ASTBoxing::ASTBoxing(std::shared_ptr<ASTExpr> expr, const Type &exprType,
                     const SourcePosition &p) : ASTExpr(p), expr_(std::move(expr)) {
    setExpressionType(exprType);
    if (auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_)) {
        init_ = init->initType() == ASTInitialization::InitType::ValueType;
    }
}

Value* ASTBoxing::getBoxValuePtr(Value *box, FunctionCodeGenerator *fg) const {
    Type type = expr_->expressionType();
    type.unbox();
    type.setOptional(false);
    return fg->getValuePtr(box, type);
}

Value* ASTBoxing::getSimpleOptional(Value *value, FunctionCodeGenerator *fg) const {
    auto structType = fg->typeHelper().llvmTypeFor(expressionType());
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = fg->builder().CreateInsertValue(undef, value, 1);
    return fg->builder().CreateInsertValue(simpleOptional, fg->generator()->optionalValue(), 0);
}

Value* ASTBoxing::getSimpleOptionalWithoutValue(FunctionCodeGenerator *fg) const {
    return fg->getSimpleOptionalWithoutValue(expressionType());
}

void ASTBoxing::getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const {
    auto metaType = fg->generator()->valueTypeMetaFor(expr_->expressionType());
    fg->builder().CreateStore(metaType, fg->getMetaTypePtr(box));

    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }

    fg->builder().CreateStore(value, getBoxValuePtr(box, fg));
}

Value* ASTBoxing::getAllocaTheBox(FunctionCodeGenerator *fg) const {
    auto box = fg->builder().CreateAlloca(fg->typeHelper().box());
    fg->builder().CreateStore(expr_->generate(fg), box);
    return box;
}

Value* ASTBoxing::getGetValueFromBox(Value *box, FunctionCodeGenerator *fg) const {
    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }
    return fg->builder().CreateLoad(getBoxValuePtr(box, fg));
}

void ASTBoxing::valueTypeInit(FunctionCodeGenerator *fg, Value *destination) const {
    auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_);
    init->setDestination(destination);
    expr_->generate(fg);
}

Value* ASTBoxToSimple::generate(FunctionCodeGenerator *fg) const {
    return getGetValueFromBox(getAllocaTheBox(fg), fg);
}

Value* ASTBoxToSimpleOptional::generate(FunctionCodeGenerator *fg) const {
    auto box = getAllocaTheBox(fg);

    auto hasNoValue = fg->getHasNoValueBoxPtr(box);
    return fg->createIfElsePhi(hasNoValue, [this, fg]() {
        return getSimpleOptionalWithoutValue(fg);
    }, [this, box, fg]() {
        return getSimpleOptional(getGetValueFromBox(box, fg), fg);
    });
}

Value* ASTSimpleToSimpleOptional::generate(FunctionCodeGenerator *fg) const {
    if (isValueTypeInit()) {
        auto value = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()));
        valueTypeInit(fg, value);
        return getSimpleOptional(value, fg);
    }
    return getSimpleOptional(expr_->generate(fg), fg);
}

Value* ASTSimpleToBox::generate(FunctionCodeGenerator *fg) const {
    auto box = fg->builder().CreateAlloca(fg->typeHelper().box());
    if (isValueTypeInit()) {
        valueTypeInit(fg, fg->getValuePtr(box, expr_->expressionType()));
    }
    else {
        getPutValueIntoBox(box, expr_->generate(fg), fg);
    }
    return fg->builder().CreateLoad(box);
}

Value* ASTSimpleOptionalToBox::generate(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto box = fg->builder().CreateAlloca(fg->typeHelper().box());

    auto hasNoValue = fg->getHasNoValue(value);

    fg->createIfElse(hasNoValue, [fg, box]() {
        fg->getMakeNoValue(box);
    }, [this, value, fg, box]() {
        getPutValueIntoBox(box, fg->builder().CreateExtractValue(value, 1), fg);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTDereference::generate(FunctionCodeGenerator *fg) const {
    return fg->builder().CreateLoad(expr_->generate(fg));
}

Value* ASTCallableBox::generate(FunctionCodeGenerator *fg) const {
    // TODO: implement
//    expr_->generate(fg);
//    fg->wr().writeInstruction(INS_CLOSURE_BOX);
//    fg->wr().writeInstruction(boxingLayer_->vtiForUse());
    throw std::logic_error("Unimplemented");
}

Value* ASTStoreTemporarily::generate(FunctionCodeGenerator *fg) const {
    auto store = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()), nullptr, "temp");
    if (isValueTypeInit()) {
        valueTypeInit(fg, store);
    }
    else {
        fg->builder().CreateStore(expr_->generate(fg), store);
    }
    return store;
}

}  // namespace EmojicodeCompiler
