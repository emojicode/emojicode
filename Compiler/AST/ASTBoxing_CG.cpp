//
//  ASTBoxing_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBoxing.hpp"
#include "ASTInitialization.hpp"
#include "Types/TypeDefinition.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/ProtocolsTableGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTUpcast::generate(FunctionCodeGenerator *fg) const {
    return fg->builder().CreateBitCast(expr_->generate(fg), fg->typeHelper().llvmTypeFor(toType_));
}

ASTBoxing::ASTBoxing(std::shared_ptr<ASTExpr> expr, const SourcePosition &p, const Type &exprType)
    : ASTExpr(p), expr_(std::move(expr)) {
    setExpressionType(exprType);
    if (auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_)) {
        init_ = init->initType() == ASTInitialization::InitType::ValueType;
    }
}

Value* ASTBoxing::getBoxValuePtr(Value *box, FunctionCodeGenerator *fg) const {
    Type type = expr_->expressionType().unboxed().unoptionalized();
    return fg->buildGetBoxValuePtr(box, type);
}

Value* ASTBoxing::getSimpleOptional(Value *value, FunctionCodeGenerator *fg) const {
    return fg->buildSimpleOptionalWithValue(value, expressionType());
}

Value* ASTBoxing::getSimpleError(llvm::Value *value, EmojicodeCompiler::FunctionCodeGenerator *fg) const {
    auto undef = llvm::UndefValue::get(fg->typeHelper().llvmTypeFor(expressionType()));
    auto error = fg->builder().CreateInsertValue(undef, fg->buildGetErrorNoError(), 0);
    return fg->builder().CreateInsertValue(error, value, 1);
}

Value* ASTBoxing::getSimpleOptionalWithoutValue(FunctionCodeGenerator *fg) const {
    return fg->buildSimpleOptionalWithoutValue(expressionType());
}

Value* ASTBoxing::getAllocaTheBox(FunctionCodeGenerator *fg) const {
    auto box = fg->createEntryAlloca(fg->typeHelper().box());
    fg->builder().CreateStore(expr_->generate(fg), box);
    return box;
}

Value* ASTBoxing::getGetValueFromBox(Value *box, FunctionCodeGenerator *fg) const {
    if (fg->typeHelper().isRemote(expr_->expressionType().unboxed().unoptionalized())) {
        auto ptrPtrType = fg->typeHelper().llvmTypeFor(expr_->expressionType())->getPointerTo()->getPointerTo();
        auto ptrPtr = fg->buildGetBoxValuePtr(box, ptrPtrType);
        return fg->builder().CreateLoad(fg->builder().CreateLoad(ptrPtr));
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

    auto hasNoValue = fg->buildHasNoValueBoxPtr(box);
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
    auto box = fg->createEntryAlloca(fg->typeHelper().box());
    if (isValueTypeInit()) {
        setBoxInfo(box, fg);
        valueTypeInit(fg, fg->buildGetBoxValuePtr(box, expr_->expressionType()));
    }
    else {
        getPutValueIntoBox(box, expr_->generate(fg), fg);
    }
    return fg->builder().CreateLoad(box);
}

Value* ASTSimpleOptionalToBox::generate(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto box = fg->createEntryAlloca(fg->typeHelper().box());

    auto hasNoValue = fg->buildHasNoValue(value);

    fg->createIfElse(hasNoValue, [fg, box]() {
        fg->buildMakeNoValue(box);
    }, [this, value, fg, box]() {
        getPutValueIntoBox(box, fg->builder().CreateExtractValue(value, 1), fg);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTSimpleErrorToBox::generate(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto box = fg->createEntryAlloca(fg->typeHelper().box());

    auto hasNoValue = fg->buildGetIsError(value);

    fg->createIfElse(hasNoValue, [this, fg, box, value]() {
        fg->buildMakeNoValue(box);
        auto ptr = fg->buildGetBoxValuePtr(box, expressionType().errorEnum());
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
    auto hasNoValue = fg->buildHasNoValueBoxPtr(box);
    return fg->createIfElsePhi(hasNoValue, [this, box, fg]() {
        auto errorEnumValue = fg->buildErrorEnumValueBoxPtr(box, expressionType().errorEnum());
        return fg->buildSimpleErrorWithError(errorEnumValue, fg->typeHelper().llvmTypeFor(expressionType()));
    }, [this, box, fg]() {
        return getSimpleError(getGetValueFromBox(box, fg), fg);
    });
}

void ASTToBox::getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const {
    setBoxInfo(box, fg);
    if (fg->typeHelper().isRemote(expr_->expressionType())) {
        auto type = fg->typeHelper().llvmTypeFor(expr_->expressionType());
        auto ptrPtr = fg->buildGetBoxValuePtr(box, type->getPointerTo()->getPointerTo());
        auto alloc = allocate(fg, type);
        fg->builder().CreateStore(value, alloc);
        fg->builder().CreateStore(alloc, ptrPtr);
    }
    else {
        fg->builder().CreateStore(value, getBoxValuePtr(box, fg));
    }
}

void ASTToBox::setBoxInfo(Value *box, FunctionCodeGenerator *fg) const {
    auto boxedFor = expressionType().boxedFor();
    if (boxedFor.type() == TypeType::Protocol || boxedFor.type() == TypeType::MultiProtocol) {
        llvm::Value *table;
        llvm::Type *type;
        if (boxedFor.type() == TypeType::MultiProtocol) {
            type = fg->typeHelper().multiprotocolConformance(boxedFor);
            table = fg->generator()->protocolsTG().multiprotocol(boxedFor, expr_->expressionType());
        }
        else {
            type = fg->typeHelper().protocolConformance();
            table = expr_->expressionType().typeDefinition()->protocolTableFor(boxedFor);
        }
        auto ptr = fg->builder().CreateBitCast(fg->buildGetBoxInfoPtr(box), type->getPointerTo()->getPointerTo());
        fg->builder().CreateStore(table, ptr);
        return;
    }
    auto boxInfo = fg->boxInfoFor(expr_->expressionType().unoptionalized());
    fg->builder().CreateStore(boxInfo, fg->buildGetBoxInfoPtr(box));
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
    auto store = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()), "temp");
    if (isValueTypeInit()) {
        valueTypeInit(fg, store);
    }
    else {
        fg->builder().CreateStore(expr_->generate(fg), store);
    }
    return store;
}

}  // namespace EmojicodeCompiler
