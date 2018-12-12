//
//  ASTBoxing_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBoxing.hpp"
#include "ASTInitialization.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/ProtocolsTableGenerator.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

Value* ASTUpcast::generate(FunctionCodeGenerator *fg) const {
    return fg->builder().CreateBitCast(expr_->generate(fg), fg->typeHelper().llvmTypeFor(toType_));
}

ASTBoxing::ASTBoxing(std::shared_ptr<ASTExpr> expr, const SourcePosition &p, const Type &exprType)
    : ASTUnaryMFForwarding(std::move(expr), p) {
    setExpressionType(exprType);
    if (auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_)) {
        init_ = init->initType() == ASTInitialization::InitType::ValueType;
    }
}

Value* ASTRebox::generate(FunctionCodeGenerator *fg) const {
    if (expressionType().boxedFor().type() == TypeType::Something) {
        auto box = expr_->generate(fg);
        auto pct = fg->typeHelper().protocolConformance();
        auto pc = fg->builder().CreateBitCast(fg->builder().CreateExtractValue(box, 0), pct->getPointerTo());
        auto bi = fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(pct, pc, 0, 2));
        return fg->builder().CreateInsertValue(box, bi, 0);
    }

    auto box = getAllocaTheBox(fg);
    auto protocolId = fg->generator()->protocolIdentifierFor(expressionType().boxedFor());
    auto conformance = fg->buildFindProtocolConformance(box, fg->builder().CreateLoad(fg->buildGetBoxInfoPtr(box)),
                                                        protocolId);
    auto confPtrTy = fg->typeHelper().protocolConformance()->getPointerTo();
    auto infoPtr = fg->buildGetBoxInfoPtr(box);
    fg->builder().CreateStore(conformance, fg->builder().CreateBitCast(infoPtr, confPtrTy->getPointerTo()));
    return fg->builder().CreateLoad(box);
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
    auto containedType = expr_->expressionType().unboxed().unoptionalized();
    if (fg->typeHelper().isRemote(containedType)) {
        auto type = fg->typeHelper().llvmTypeFor(containedType);
        auto ptrPtr = fg->buildGetBoxValuePtr(box, type->getPointerTo()->getPointerTo());
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
        valueTypeInit(fg, buildStoreAddress(box, fg));
    }
    else {
        getPutValueIntoBox(box, expr_->generate(fg), fg);
    }
    return fg->builder().CreateLoad(box);
}

Value* ASTSimpleOptionalToBox::generate(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto box = fg->createEntryAlloca(fg->typeHelper().box());

    auto hasNoValue = fg->buildOptionalHasNoValue(value, expr_->expressionType());

    fg->createIfElse(hasNoValue, [fg, box]() {
        fg->buildMakeNoValue(box);
    }, [this, value, fg, box]() {
        getPutValueIntoBox(box, fg->buildGetOptionalValue(value, expr_->expressionType()), fg);
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

Value* ASTToBox::buildStoreAddress(Value *box, FunctionCodeGenerator *fg) const {
    auto containedType = expr_->expressionType().unboxed().unoptionalized();
    if (fg->typeHelper().isRemote(containedType)) {
        auto containedTypeLlvm = fg->typeHelper().llvmTypeFor(containedType);
        auto mngType = fg->typeHelper().managable(containedTypeLlvm);
        auto ctPtrPtr = containedTypeLlvm->getPointerTo()->getPointerTo();
        auto boxPtr1 = fg->buildGetBoxValuePtr(box, ctPtrPtr);
        auto boxPtr2 = fg->buildGetBoxValuePtrAfter(box, mngType->getPointerTo(), mngType->getPointerTo());
        auto alloc = allocate(fg, mngType);
        auto valuePtr = fg->managableGetValuePtr(alloc);
        // The first element in the value area is a direct pointer to the struct.
        fg->builder().CreateStore(valuePtr, boxPtr1);
        // The second is a pointer to the allocated object for management.
        fg->builder().CreateStore(alloc, boxPtr2);
        return valuePtr;
    }
    return getBoxValuePtr(box, fg);
}

void ASTToBox::getPutValueIntoBox(Value *box, Value *value, FunctionCodeGenerator *fg) const {
    setBoxInfo(box, fg);
    fg->builder().CreateStore(value, buildStoreAddress(box, fg));
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

Value* ASTBoxReferenceToReference::generate(FunctionCodeGenerator *fg) const {
    auto containedType = expr_->expressionType().unboxed().unoptionalized();
    auto type = fg->typeHelper().llvmTypeFor(containedType);
    if (fg->typeHelper().isRemote(containedType)) {
        auto ptrPtr = fg->buildGetBoxValuePtr(expr_->generate(fg), type->getPointerTo()->getPointerTo());
        return fg->builder().CreateLoad(ptrPtr);
    }
    return fg->buildGetBoxValuePtr(expr_->generate(fg), type->getPointerTo());
}

void ASTBoxReferenceToReference::mutateReference(ExpressionAnalyser *analyser) {
    expr_->mutateReference(analyser);
}

Value* ASTDereference::generate(FunctionCodeGenerator *fg) const {
    auto ptr = expr_->generate(fg);
    auto val = fg->builder().CreateLoad(ptr);
    if (expressionType().isManaged()) {
        fg->retain(fg->isManagedByReference(expressionType()) ? ptr : val, expressionType());
    }
    return handleResult(fg, val, ptr);
}

void ASTDereference::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->take(expr_.get());
}

Value* ASTBoxReferenceToSimple::generate(FunctionCodeGenerator *fg) const {
    auto box = expr_->generate(fg);
    auto containedType = expr_->expressionType().unboxed().unoptionalized();
    llvm::Value *valuePtr;

    if (fg->typeHelper().isRemote(containedType)) {
        auto type = fg->typeHelper().llvmTypeFor(containedType);
        auto ptrPtr = fg->buildGetBoxValuePtr(box, type->getPointerTo()->getPointerTo());
        valuePtr = fg->builder().CreateLoad(ptrPtr);
    }
    else {
        valuePtr = getBoxValuePtr(box, fg);
    }

    auto val = fg->builder().CreateLoad(valuePtr);
    if (expressionType().isManaged()) {
        fg->retain(fg->isManagedByReference(expressionType()) ? valuePtr : val, expressionType());
    }
    return handleResult(fg, val, valuePtr);
}

}  // namespace EmojicodeCompiler
