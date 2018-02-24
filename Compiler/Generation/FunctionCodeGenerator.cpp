//
//  FunctionCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Types/Enum.hpp"
#include "AST/ASTStatements.hpp"
#include "Declarator.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Package/Package.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>

namespace EmojicodeCompiler {

void FunctionCodeGenerator::generate() {
    auto basicBlock = llvm::BasicBlock::Create(generator()->context(), "entry", function_);
    builder_.SetInsertPoint(basicBlock);

    declareArguments(function_);

    fn_->ast()->generate(this);

    if (llvm::verifyFunction(*function_, &llvm::outs())) {

    }
}

Compiler* FunctionCodeGenerator::compiler() const {
    return generator()->package()->compiler();
}

void FunctionCodeGenerator::declareArguments(llvm::Function *function) {
    unsigned int i = 0;
    auto it = function->args().begin();
    if (hasThisArgument(fn_->functionType())) {
        (it++)->setName("this");
    }
    for (auto arg : fn_->arguments()) {
        auto &llvmArg = *(it++);
        scoper_.getVariable(i++) = LocalVariable(false, &llvmArg);
        llvmArg.setName(utf8(arg.variableName));
    }
}

llvm::Value* FunctionCodeGenerator::sizeOfReferencedType(llvm::PointerType *type) {
    auto one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 1);
    type->print(llvm::outs(), true);
    llvm::outs() << "\n";
    auto sizeg = builder().CreateGEP(llvm::ConstantPointerNull::getNullValue(type), one);
    return builder().CreatePtrToInt(sizeg, llvm::Type::getInt64Ty(generator()->context()));
}

llvm::Value* FunctionCodeGenerator::sizeOf(llvm::Type *type) {
    return sizeOfReferencedType(type->getPointerTo());
}

llvm::Value* FunctionCodeGenerator::getMetaFromObject(llvm::Value *object) {
    std::vector<Value *> idx{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),  // object
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),  // classMeta
    };
    return builder().CreateLoad(builder().CreateGEP(object, idx), "meta");
}

llvm::Value* FunctionCodeGenerator::getHasNoValueBoxPtr(llvm::Value *box) {
    auto null = llvm::Constant::getNullValue(typeHelper().valueTypeMetaPtr());
    return builder().CreateICmpEQ(builder().CreateLoad(getMetaTypePtr(box)), null);
}

Value* FunctionCodeGenerator::getMetaTypePtr(Value *box) {
    std::vector<Value *> idx{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
    };
    return builder().CreateGEP(box, idx);
}

llvm::Value* FunctionCodeGenerator::getHasNoValueBox(llvm::Value *box) {
    auto vf = builder().CreateExtractValue(box, 0);
    return builder().CreateICmpEQ(vf, llvm::Constant::getNullValue(vf->getType()));
}

Value* FunctionCodeGenerator::getHasNoValue(llvm::Value *simpleOptional) {
    auto vf = builder().CreateExtractValue(simpleOptional, 0);
    return builder().CreateICmpEQ(vf, generator()->optionalNoValue());
}

Value* FunctionCodeGenerator::getSimpleOptionalWithoutValue(const Type &type) {
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    return builder().CreateInsertValue(undef, generator()->optionalNoValue(), 0);
}

Value* FunctionCodeGenerator::getBoxOptionalWithoutValue() {
    auto undef = llvm::UndefValue::get(typeHelper().box());
    return builder().CreateInsertValue(undef, llvm::Constant::getNullValue(typeHelper().valueTypeMetaPtr()), 0);
}

Value* FunctionCodeGenerator::getSimpleOptionalWithValue(llvm::Value *value, const Type &type) {
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = builder().CreateInsertValue(undef, value, 1);
    return builder().CreateInsertValue(simpleOptional, generator()->optionalValue(), 0);
}

Value* FunctionCodeGenerator::getValuePtr(Value *box, const Type &type) {
    auto llvmType = typeHelper().llvmTypeFor(type)->getPointerTo();
    return getValuePtr(box, llvmType);
}

Value* FunctionCodeGenerator::getValuePtr(Value *box, llvm::Type *llvmType) {
    std::vector<Value *> idx2{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 1),
    };
    return builder().CreateBitCast(builder().CreateGEP(box, idx2), llvmType);
}

Value* FunctionCodeGenerator::getObjectMetaPtr(Value *object) {
    auto metaIdx = std::vector<llvm::Value *> {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0)
    };
    return builder().CreateGEP(object, metaIdx);
}

Value* FunctionCodeGenerator::getMakeNoValue(Value *box) {
    auto metaType = llvm::Constant::getNullValue(typeHelper().valueTypeMetaPtr());
    return builder().CreateStore(metaType, getMetaTypePtr(box));
}

void FunctionCodeGenerator::createIfElseBranchCond(llvm::Value *cond, const std::function<bool()> &then,
                                   const std::function<bool()> &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto success = llvm::BasicBlock::Create(generator()->context(), "then", function);
    auto fail = llvm::BasicBlock::Create(generator()->context(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(generator()->context(), "cont", function);

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

void FunctionCodeGenerator::createIfElse(llvm::Value *cond, const std::function<void()> &then,
                                         const std::function<void()> &otherwise) {
    createIfElseBranchCond(cond, [then]() { then(); return true; }, [otherwise]() { otherwise(); return true; });
}

llvm::Value* FunctionCodeGenerator::createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
                                              const std::function<llvm::Value *()> &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto thenBlock = llvm::BasicBlock::Create(generator()->context(), "then", function);
    auto otherwiseBlock = llvm::BasicBlock::Create(generator()->context(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(generator()->context(), "cont", function);

    builder().CreateCondBr(cond, thenBlock, otherwiseBlock);

    builder().SetInsertPoint(thenBlock);
    auto thenValue = then();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(otherwiseBlock);
    auto otherwiseValue = otherwise();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(mergeBlock);
    auto phi = builder().CreatePHI(thenValue->getType(), 2);
    phi->addIncoming(thenValue, thenBlock);
    phi->addIncoming(otherwiseValue, otherwiseBlock);
    return phi;
}

std::pair<llvm::Value*, llvm::Value*>
    FunctionCodeGenerator::createIfElsePhi(llvm::Value* cond, const FunctionCodeGenerator::PairIfElseCallback &then,
                                           const FunctionCodeGenerator::PairIfElseCallback &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto thenBlock = llvm::BasicBlock::Create(generator()->context(), "then", function);
    auto otherwiseBlock = llvm::BasicBlock::Create(generator()->context(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(generator()->context(), "cont", function);

    builder().CreateCondBr(cond, thenBlock, otherwiseBlock);

    builder().SetInsertPoint(thenBlock);
    auto thenValue = then();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(otherwiseBlock);
    auto otherwiseValue = otherwise();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(mergeBlock);
    auto phi1 = builder().CreatePHI(thenValue.first->getType(), 2);
    phi1->addIncoming(thenValue.first, thenBlock);
    phi1->addIncoming(otherwiseValue.first, otherwiseBlock);
    auto phi2 = builder().CreatePHI(thenValue.second->getType(), 2);
    phi2->addIncoming(thenValue.second, thenBlock);
    phi2->addIncoming(otherwiseValue.second, otherwiseBlock);
    return std::make_pair(phi1, phi2);
}

llvm::Value* FunctionCodeGenerator::int16(int16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(generator()->context()), value);
}

llvm::Value* FunctionCodeGenerator::int32(int32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), value);
}

llvm::Value* FunctionCodeGenerator::int64(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(generator()->context()), value);
}

llvm::Value* FunctionCodeGenerator::alloc(llvm::PointerType *type) {
    auto alloc = builder().CreateCall(generator()->declarator().runTimeNew(), sizeOfReferencedType(type), "alloc");
    return builder().CreateBitCast(alloc, type);
}

}  // namespace EmojicodeCompiler
