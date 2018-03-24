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
    Type type = expr_->expressionType().unboxed().unoptionalized();
    return fg->getValuePtr(box, type);
}

Value* ASTBoxing::getSimpleOptional(Value *value, FunctionCodeGenerator *fg) const {
    return fg->getSimpleOptionalWithValue(value, expressionType());
}

llvm::Value * ASTBoxing::getSimpleError(llvm::Value *value, EmojicodeCompiler::FunctionCodeGenerator *fg) const {
    auto undef = llvm::UndefValue::get(fg->typeHelper().llvmTypeFor(expressionType()));
    auto error = fg->builder().CreateInsertValue(undef, fg->getErrorNoError(), 0);
    return fg->builder().CreateInsertValue(error, value, 1);
}

Value* ASTBoxing::getSimpleOptionalWithoutValue(FunctionCodeGenerator *fg) const {
    return fg->getSimpleOptionalWithoutValue(expressionType());
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
    return getSimpleOptional(expr_->generate(fg), fg);
}

Value* ASTSimpleToBox::generate(FunctionCodeGenerator *fg) const {
    auto box = fg->builder().CreateAlloca(fg->typeHelper().box());
    if (isValueTypeInit()) {
        setBoxMeta(box, fg);
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

Value* ASTSimpleErrorToBox::generate(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto box = fg->builder().CreateAlloca(fg->typeHelper().box());

    auto hasNoValue = fg->getIsError(value);

    fg->createIfElse(hasNoValue, [this, fg, box, value]() {
        fg->getMakeNoValue(box);
        auto ptr = fg->getValuePtr(box, expressionType().errorEnum());
        fg->builder().CreateStore(fg->builder().CreateExtractValue(value, 0), ptr);
    }, [this, value, fg, box]() {
        getPutValueIntoBox(box, fg->builder().CreateExtractValue(value, 1), fg);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTSimpleToSimpleError::generate(FunctionCodeGenerator *fg) const {
    return getSimpleError(expr_->generate(fg), fg);
}

Value* ASTBoxToSimpleError::generate(FunctionCodeGenerator *fg) const {
    auto box = getAllocaTheBox(fg);
    auto hasNoValue = fg->getHasNoValueBoxPtr(box);
    return fg->createIfElsePhi(hasNoValue, [this, box, fg]() {
        auto errorEnumValue = fg->getErrorEnumValueBoxPtr(box, expressionType().errorEnum());
        return fg->getSimpleErrorWithError(errorEnumValue, fg->typeHelper().llvmTypeFor(expressionType()));
    }, [this, box, fg]() {
        return getSimpleError(getGetValueFromBox(box, fg), fg);
    });
}

void ASTToBox::getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const {
    setBoxMeta(box, fg);
    if (expr_->expressionType().remotelyStored()) {
        // TODO: Implement
    }
    fg->builder().CreateStore(value, getBoxValuePtr(box, fg));
}

void ASTToBox::setBoxMeta(Value *box, FunctionCodeGenerator *fg) const {
    if (toType().type() == TypeType::Protocol) {
        auto ptr = fg->builder().CreateBitCast(fg->getMetaTypePtr(box),
                                               fg->typeHelper().protocolConformance()->getPointerTo()->getPointerTo());
        fg->builder().CreateStore(expr_->expressionType().typeDefinition()->protocolTableFor(toType()), ptr);
        return;
    }
    auto metaType = fg->generator()->valueTypeMetaFor(expr_->expressionType().unoptionalized());
    fg->builder().CreateStore(metaType, fg->getMetaTypePtr(box));
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
