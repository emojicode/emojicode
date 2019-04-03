//
//  FunctionCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "FunctionCodeGenerator.hpp"
#include "AST/ASTStatements.hpp"
#include "Compiler.hpp"
#include "RunTimeHelper.hpp"
#include "Functions/Function.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Package/Package.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

namespace EmojicodeCompiler {

FunctionCodeGenerator::FunctionCodeGenerator(Function *function, llvm::Function *llvmFunc, CodeGenerator *generator)
    : fn_(function), function_(llvmFunc), scoper_(function->variableCount()),
      generator_(generator), builder_(generator->context()), typeContext_(std::make_unique<TypeContext>(fn_->typeContext())) {}

FunctionCodeGenerator::FunctionCodeGenerator(llvm::Function *llvmFunc, CodeGenerator *generator,
                                             std::unique_ptr<TypeContext> tc)
    : fn_(nullptr), function_(llvmFunc), scoper_(0), generator_(generator), builder_(generator->context()),
      typeContext_(std::move(tc)) {}

void FunctionCodeGenerator::generate() {
    createEntry();

    declareArguments(function_);

    fn_->ast()->generate(this);

    if (llvm::verifyFunction(*function_, &llvm::outs())) {
        auto ows = function()->owner() != nullptr ? function()->owner()->type().toString(fn_->typeContext()) : "";
        printf("\nDetected in: %s%s (%s)\n=============\n", ows.c_str(), utf8(fn_->name()).c_str(),
               function_->getName().str().c_str());
    }
}

void FunctionCodeGenerator::createEntry() {
    auto basicBlock = llvm::BasicBlock::Create(ctx(), "entry", function_);
    builder_.SetInsertPoint(basicBlock);
}

Compiler* FunctionCodeGenerator::compiler() const {
    return generator()->compiler();
}

void FunctionCodeGenerator::declareArguments(llvm::Function *function) {
    unsigned int i = 0;
    auto it = function->args().begin();
    if (hasThisArgument(fn_)) {
        (it++)->setName("this");
    }
    for (auto &arg : fn_->parameters()) {
        auto &llvmArg = *(it++);
        setVariable(i++, &llvmArg);
        llvmArg.setName(utf8(arg.name));
    }

    if ((fn_->functionType() == FunctionType::ValueTypeInitializer ||
         fn_->functionType() == FunctionType::ObjectInitializer) &&
        typeHelper().storesGenericArgs(typeContext_->calleeType())) {
        auto llvmArg = (it++);
        llvmArg->setName("genericArgs");
        builder().CreateStore(llvmArg, genericArgsPtr());
    }

    if (!fn_->genericParameters().empty()) {
        auto llvmArg = (it++);
        llvmArg->setName("fnGenericArgs");
        functionGenericArgs_ = llvmArg;
    }

    if (fn_->errorProne()) {
        it->setName("error");
    }
}

const Type& FunctionCodeGenerator::calleeType() const {
    return typeContext_->calleeType();
}

void FunctionCodeGenerator::setVariable(size_t id, llvm::Value *value, const llvm::Twine &name) {
    auto alloca = createEntryAlloca(value->getType(), name);
    builder().CreateStore(value, alloca);
    scoper_.getVariable(id) = alloca;
}

void FunctionCodeGenerator::buildErrorReturn() {
    if (llvmReturnType()->isVoidTy()) {
        builder().CreateRetVoid();
    }
    else {
        builder().CreateRet(llvm::UndefValue::get(llvmReturnType()));
    }
}

llvm::Value* FunctionCodeGenerator::sizeOfReferencedType(llvm::PointerType *ptrType) {
    auto one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx()), 1);
    auto sizeg = builder().CreateGEP(llvm::ConstantPointerNull::getNullValue(ptrType), one);
    return builder().CreatePtrToInt(sizeg, llvm::Type::getInt64Ty(ctx()));
}

llvm::Value* FunctionCodeGenerator::sizeOf(llvm::Type *type) {
    return sizeOfReferencedType(type->getPointerTo());
}

Value* FunctionCodeGenerator::buildGetBoxInfoPtr(Value *box) {
    return builder().CreateConstInBoundsGEP2_32(typeHelper().box(), box, 0, 0);
}

llvm::Value* FunctionCodeGenerator::buildGetClassInfoPtrFromObject(Value *object) {
    return builder().CreateConstInBoundsGEP2_32(llvm::cast<llvm::PointerType>(object->getType())->getElementType(),
                                                object, 0, 1); // classInfo*
}

llvm::Value* FunctionCodeGenerator::buildGetClassInfoFromObject(llvm::Value *object) {
    return builder().CreateLoad(buildGetClassInfoPtrFromObject(object), "info");
}

llvm::Value* FunctionCodeGenerator::buildHasNoValueBoxPtr(llvm::Value *box) {
    return builder().CreateIsNull(builder().CreateLoad(buildGetBoxInfoPtr(box)));
}

llvm::Value* FunctionCodeGenerator::buildHasNoValueBox(llvm::Value *box) {
    return builder().CreateIsNull(builder().CreateExtractValue(box, 0));
}

Value* FunctionCodeGenerator::buildOptionalHasNoValue(llvm::Value *simpleOptional, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return builder().CreateIsNull(simpleOptional);
    }
    auto vf = builder().CreateExtractValue(simpleOptional, 0);
    return builder().CreateICmpEQ(vf, llvm::ConstantInt::getFalse(ctx()));
}

Value* FunctionCodeGenerator::buildOptionalHasValue(llvm::Value *simpleOptional, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return builder().CreateIsNotNull(simpleOptional);
    }
    return builder().CreateExtractValue(simpleOptional, 0);
}

Value* FunctionCodeGenerator::buildOptionalHasValuePtr(llvm::Value *simpleOptional, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return builder().CreateIsNotNull(simpleOptional);
    }
    auto ptype = llvm::cast<llvm::PointerType>(simpleOptional->getType())->getElementType();
    return builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(ptype, simpleOptional, 0, 0));
}

Value* FunctionCodeGenerator::buildGetOptionalValuePtr(llvm::Value *simpleOptional, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return builder().CreateLoad(simpleOptional);
    }
    auto ptype = llvm::cast<llvm::PointerType>(simpleOptional->getType())->getElementType();
    return builder().CreateConstInBoundsGEP2_32(ptype, simpleOptional, 0, 1);
}

Value* FunctionCodeGenerator::buildSimpleOptionalWithoutValue(const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return llvm::Constant::getNullValue(typeHelper().llvmTypeFor(type.optionalType()));
    }
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    return builder().CreateInsertValue(undef, llvm::ConstantInt::getFalse(ctx()), 0);
}

Value* FunctionCodeGenerator::buildBoxWithoutValue() {
    auto undef = llvm::UndefValue::get(typeHelper().box());
    return builder().CreateInsertValue(undef, llvm::Constant::getNullValue(typeHelper().boxInfo()->getPointerTo()), 0);
}

Value* FunctionCodeGenerator::buildSimpleOptionalWithValue(llvm::Value *value, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return value;
    }
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = builder().CreateInsertValue(undef, value, 1);
    return builder().CreateInsertValue(simpleOptional, llvm::ConstantInt::getTrue(ctx()), 0);
}

Value* FunctionCodeGenerator::buildGetOptionalValue(llvm::Value *value, const Type &type) {
    if (type.storageType() == StorageType::PointerOptional) {
        return value;
    }
    return builder().CreateExtractValue(value, 1);
}

Value* FunctionCodeGenerator::buildGetBoxValuePtr(Value *box, const Type &type) {
    auto llvmType = typeHelper().llvmTypeFor(type)->getPointerTo();
    return buildGetBoxValuePtr(box, llvmType);
}

Value* FunctionCodeGenerator::buildGetBoxValuePtr(Value *box, llvm::Type *llvmType) {
    return builder().CreateBitCast(builder().CreateConstInBoundsGEP2_32(typeHelper().box(), box, 0, 1), llvmType);
}

llvm::Value* FunctionCodeGenerator::buildGetBoxValuePtrAfter(llvm::Value *box, llvm::Type *llvmType,
                                                             llvm::Type *after) {
    auto val = builder().CreateConstInBoundsGEP2_32(typeHelper().box(), box, 0, 1);
    auto strType = llvm::StructType::get(after, llvmType);
    auto strPtr = builder().CreateBitCast(val, strType->getPointerTo());
    return builder().CreateConstInBoundsGEP2_32(strType, strPtr, 0, 1);
}

void FunctionCodeGenerator::createIfElseBranchCond(llvm::Value *cond, const std::function<bool()> &then,
                                   const std::function<bool()> &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto success = llvm::BasicBlock::Create(ctx(), "then", function);
    auto fail = llvm::BasicBlock::Create(ctx(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(ctx(), "cont", function);

    builder().CreateCondBr(cond, success, fail);

    builder().SetInsertPoint(success);
    if (then()) {
        builder().CreateBr(mergeBlock);
    }

    builder().SetInsertPoint(fail);
    if (otherwise()) {
        builder().CreateBr(mergeBlock);
    }
    builder().SetInsertPoint(mergeBlock);
}

void FunctionCodeGenerator::createIf(llvm::Value *cond, const std::function<void()> &then) {
    auto function = builder().GetInsertBlock()->getParent();
    auto thenBlock = llvm::BasicBlock::Create(ctx(), "then", function);
    auto cont = llvm::BasicBlock::Create(ctx(), "cont", function);

    builder().CreateCondBr(cond, thenBlock, cont);
    builder().SetInsertPoint(thenBlock);
    then();
    builder().CreateBr(cont);
    builder().SetInsertPoint(cont);
}

llvm::BasicBlock* FunctionCodeGenerator::createBlock(const llvm::Twine &name) {
    auto function = builder().GetInsertBlock()->getParent();
    return llvm::BasicBlock::Create(ctx(), name, function);
}

void FunctionCodeGenerator::createIfElse(llvm::Value *cond, const std::function<void()> &then,
                                         const std::function<void()> &otherwise) {
    createIfElseBranchCond(cond, [then]() { then(); return true; }, [otherwise]() { otherwise(); return true; });
}

llvm::Value* FunctionCodeGenerator::createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
                                              const std::function<llvm::Value *()> &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto thenBlock = llvm::BasicBlock::Create(ctx(), "then", function);
    auto otherwiseBlock = llvm::BasicBlock::Create(ctx(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(ctx(), "cont", function);

    builder().CreateCondBr(cond, thenBlock, otherwiseBlock);

    builder().SetInsertPoint(thenBlock);
    auto thenValue = then();
    auto thenIncoming = builder().GetInsertBlock();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(otherwiseBlock);
    auto otherwiseValue = otherwise();
    auto otherwiseIncoming = builder().GetInsertBlock();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(mergeBlock);
    auto phi = builder().CreatePHI(thenValue->getType(), 2);
    phi->addIncoming(thenValue, thenIncoming);
    phi->addIncoming(otherwiseValue, otherwiseIncoming);
    return phi;
}

std::pair<llvm::Value*, llvm::Value*>
    FunctionCodeGenerator::createIfElsePhi(llvm::Value* cond, const FunctionCodeGenerator::PairIfElseCallback &then,
                                           const FunctionCodeGenerator::PairIfElseCallback &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto thenBlock = llvm::BasicBlock::Create(ctx(), "then", function);
    auto otherwiseBlock = llvm::BasicBlock::Create(ctx(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(ctx(), "cont", function);

    builder().CreateCondBr(cond, thenBlock, otherwiseBlock);

    builder().SetInsertPoint(thenBlock);
    auto thenValue = then();
    auto thenIncoming = builder().GetInsertBlock();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(otherwiseBlock);
    auto otherwiseValue = otherwise();
    auto otherwiseIncoming = builder().GetInsertBlock();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(mergeBlock);
    auto phi1 = builder().CreatePHI(thenValue.first->getType(), 2);
    phi1->addIncoming(thenValue.first, thenIncoming);
    phi1->addIncoming(otherwiseValue.first, otherwiseIncoming);
    auto phi2 = builder().CreatePHI(thenValue.second->getType(), 2);
    phi2->addIncoming(thenValue.second, thenIncoming);
    phi2->addIncoming(otherwiseValue.second, otherwiseIncoming);
    return std::make_pair(phi1, phi2);
}

llvm::ConstantInt* FunctionCodeGenerator::int8(int8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx()), value);
}

llvm::ConstantInt* FunctionCodeGenerator::int16(int16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(ctx()), value);
}

llvm::ConstantInt* FunctionCodeGenerator::int32(int32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx()), value);
}

llvm::ConstantInt* FunctionCodeGenerator::int64(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx()), value);
}

llvm::Value* FunctionCodeGenerator::alloc(llvm::PointerType *type) {
    auto alloc = builder().CreateCall(generator()->runTime().alloc(), sizeOfReferencedType(type), "alloc");
    return builder().CreateBitCast(alloc, type);
}

llvm::Value* FunctionCodeGenerator::stackAlloc(llvm::PointerType *type) {
    auto structType = llvm::StructType::get(llvm::Type::getInt64Ty(ctx()), type->getElementType());
    auto ptr = createEntryAlloca(structType);

    builder().CreateStore(int64(1), builder().CreateConstInBoundsGEP2_32(structType, ptr, 0, 0));
    auto object = builder().CreateConstInBoundsGEP2_32(structType, ptr, 0, 1);
    auto controlBlockField = builder().CreateConstInBoundsGEP2_32(type->getElementType(), object, 0, 0);
    builder().CreateStore(llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(ctx())),
                          controlBlockField);
    return object;
}

llvm::Value* FunctionCodeGenerator::managableGetValuePtr(llvm::Value *managablePtr) {
    auto elementType = llvm::dyn_cast<llvm::PointerType>(managablePtr->getType())->getElementType();
    return builder().CreateConstInBoundsGEP2_32(elementType, managablePtr, 0, 1);
}

llvm::Value* FunctionCodeGenerator::createEntryAlloca(llvm::Type *type, const llvm::Twine &name) {
    llvm::IRBuilder<> builder(&function_->getEntryBlock(), function_->getEntryBlock().begin());
    return builder.CreateAlloca(type, nullptr, name);
}

llvm::Constant* FunctionCodeGenerator::boxInfoFor(const Type &type) {
    return generator()->boxInfoFor(type);
}

void TemporaryObjectsManager::releaseTemporaryObjects(FunctionCodeGenerator *fg, bool clearQueue, bool skipLast) {
    if (temporaryObjects_.empty()) return;
    auto end = skipLast ? temporaryObjects_.end() - 1 : temporaryObjects_.end();
    for (auto it = temporaryObjects_.begin(); it < end; it++) {
        fg->release(it->value, it->type);
    }
    if (clearQueue) {
        temporaryObjects_.clear();
    }
}

void FunctionCodeGenerator::release(llvm::Value *value, const Type &otype) {
    auto type = otype.resolveOnSuperArgumentsAndConstraints(*typeContext_);
    if (type.type() == TypeType::Class || type.type() == TypeType::Someobject) {
        auto opc = builder().CreateBitCast(value, llvm::Type::getInt8PtrTy(ctx()));
        builder().CreateCall(generator()->runTime().release(), opc);
    }
    else if (type.type() == TypeType::ValueType && type.valueType() == compiler()->sMemory) {
        builder().CreateCall(generator()->runTime().releaseMemory(), value);
    }
    else if (type.type() == TypeType::ValueType) {
        builder().CreateCall(type.valueType()->deinitializer()->unspecificReification().function, value);
    }
    else if (type.type() == TypeType::Optional) {
        if (isManagedByReference(type)) {
            createIf(buildOptionalHasValuePtr(value, type), [&] {
                release(buildGetOptionalValuePtr(value, type), type.optionalType());
            });
        }
        else {
            createIf(buildOptionalHasValue(value, type), [&] {
                release(buildGetOptionalValue(value, type), type.optionalType());
            });
        }
    }
    else if (type.type() == TypeType::Box) {
        auto boxInfo = builder().CreateLoad(buildGetBoxInfoPtr(value));
        if (type.unboxed().type() == TypeType::Optional || type.unboxed().type() == TypeType::Something) {
            auto null = llvm::ConstantPointerNull::get(typeHelper().boxInfo()->getPointerTo());
            createIf(builder().CreateICmpNE(boxInfo, null), [&] {
                manageBox(false, boxInfo, value, type);
            });
        }
        else {
            manageBox(false, boxInfo, value, type);
        }
    }
    else if (type.type() == TypeType::Callable) {
        builder().CreateCall(generator()->runTime().releaseCapture(), builder().CreateExtractValue(value, 1));
    }
}

void FunctionCodeGenerator::retain(llvm::Value *value, const Type &otype) {
    auto type = otype.resolveOnSuperArgumentsAndConstraints(*typeContext_);
    if (type.type() == TypeType::Class || type.type() == TypeType::Someobject) {
        auto opc = builder().CreateBitCast(value, llvm::Type::getInt8PtrTy(ctx()));
        builder().CreateCall(generator()->runTime().retain(), { opc });
    }
    else if (type.type() == TypeType::ValueType && type.valueType() == compiler()->sMemory) {
        builder().CreateCall(generator()->runTime().retainMemory(), value);
    }
    else if (type.type() == TypeType::Callable) {
        builder().CreateCall(generator()->runTime().retain(), builder().CreateExtractValue(value, 1));
    }
    else if (type.type() == TypeType::ValueType) {
        builder().CreateCall(type.valueType()->copyRetain()->unspecificReification().function, value);
    }
    else if (type.type() == TypeType::Optional) {
        if (isManagedByReference(type)) {
            createIf(buildOptionalHasValuePtr(value, type), [&] {
                retain(buildGetOptionalValuePtr(value, type), type.optionalType());
            });
        }
        else {
            createIf(buildOptionalHasValue(value, type), [&] {
                retain(buildGetOptionalValue(value, type), type.optionalType());
            });
        }
    }
    else if (type.type() == TypeType::Box) {
        auto boxInfo = builder().CreateLoad(buildGetBoxInfoPtr(value));
        if (type.unboxed().type() == TypeType::Optional || type.unboxed().type() == TypeType::Something) {
            auto null = llvm::ConstantPointerNull::get(typeHelper().boxInfo()->getPointerTo());
            createIf(builder().CreateICmpNE(boxInfo, null), [&] {
                manageBox(true, boxInfo, value, type);
            });
        }
        else {
            manageBox(true, boxInfo, value, type);
        }
    }
}

void FunctionCodeGenerator::manageBox(bool retain, llvm::Value *boxInfo, llvm::Value *value, const Type &type) {
    llvm::Value *fnPtr;
    if (type.boxedFor().type() == TypeType::Protocol) {
        auto conf = builder().CreateBitCast(boxInfo, typeHelper().protocolConformance()->getPointerTo());
        fnPtr = builder().CreateConstInBoundsGEP2_32(typeHelper().protocolConformance(), conf, 0, retain ? 3 : 4);
    }
    else {
        fnPtr = builder().CreateConstInBoundsGEP2_32(typeHelper().boxInfo(), boxInfo, 0, retain ? 1 : 2);
    }
    auto call = builder().CreateCall(builder().CreateLoad(fnPtr, retain ? "retain" : "release"), value);
    call->addParamAttr(0, llvm::Attribute::NoCapture);
    call->addParamAttr(0, llvm::Attribute::ReadOnly);
    call->addAttribute(llvm::AttributeList::FunctionIndex, llvm::Attribute::NoUnwind);
}

bool FunctionCodeGenerator::isManagedByReference(const Type &type) const {
    return (type.type() == TypeType::ValueType && !type.valueType()->isPrimitive()) || type.type() == TypeType::Box
        || (type.type() == TypeType::Optional && isManagedByReference(type.optionalType()));
}

void FunctionCodeGenerator::releaseByReference(llvm::Value *ptr, const Type &type) {
    release(isManagedByReference(type) ? ptr : builder().CreateLoad(ptr), type);
}

llvm::Value* FunctionCodeGenerator::buildFindProtocolConformance(llvm::Value *box, llvm::Value *boxInfo,
                                                                 llvm::Value *protocolRTTI) {
    auto objBoxInfo = builder().CreateBitCast(generator()->runTime().boxInfoForObjects(),
                                              typeHelper().boxInfo()->getPointerTo());
    auto conformanceEntries = createIfElsePhi(builder().CreateICmpEQ(boxInfo, objBoxInfo), [&]() {
        auto obj = builder().CreateLoad(buildGetBoxValuePtr(box, typeHelper().someobject()->getPointerTo()));
        auto classInfo = buildGetClassInfoFromObject(obj);
        return builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(typeHelper().classInfo(), classInfo, 0, 2));
    }, [&] {
        auto conformanceEntriesPtr = builder().CreateConstInBoundsGEP2_32(typeHelper().boxInfo(), boxInfo, 0, 3);
        return builder().CreateLoad(conformanceEntriesPtr);
    });

    return builder().CreateCall(generator()->runTime().findProtocolConformance(),
                                    { conformanceEntries, protocolRTTI });
}

llvm::Value* FunctionCodeGenerator::instanceVariablePointer(size_t id) {
    auto callee = typeContext_->calleeType();
    auto offset = callee.type() != TypeType::NoReturn ? (callee.type() == TypeType::Class ? 2 : 0) +
                    (typeHelper().storesGenericArgs(callee) ? 1 : 0) : 0;
    auto type = llvm::cast<llvm::PointerType>(thisValue()->getType())->getElementType();
    return builder().CreateConstInBoundsGEP2_32(type, thisValue(), 0, offset + id);
}

llvm::Value* FunctionCodeGenerator::genericArgsPtr() {
    auto callee = typeContext_->calleeType();
    assert(typeHelper().storesGenericArgs(callee));
    auto type = llvm::cast<llvm::PointerType>(thisValue()->getType())->getElementType();
    return builder().CreateConstInBoundsGEP2_32(type, thisValue(), 0, callee.type() == TypeType::Class ? 2 : 0);
}

FunctionCodeGenerator::~FunctionCodeGenerator() = default;

}  // namespace EmojicodeCompiler
