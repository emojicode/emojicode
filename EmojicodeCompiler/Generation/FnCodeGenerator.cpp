//
//  FnCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../Types/Enum.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Verifier.h>
#include "../AST/ASTStatements.hpp"
#include "../Functions/BoxingLayer.hpp"
#include "FnCodeGenerator.hpp"
#include <cstdlib>

namespace EmojicodeCompiler {

void FnCodeGenerator::generate() {
    auto basicBlock = llvm::BasicBlock::Create(generator()->context(), "entry", fn_->llvmFunction());
    builder_.SetInsertPoint(basicBlock);

    declareArguments(fn_->llvmFunction());

    fn_->ast()->generate(this);

    if (llvm::verifyFunction(*fn_->llvmFunction(), &llvm::outs())) {

    }
}

void FnCodeGenerator::declareArguments(llvm::Function *function) {
    unsigned int i = 0;
    auto it = function->args().begin();
    if (isSelfAllowed(fn_->functionType())) {
        (it++)->setName("this");
    }
    for (auto arg : fn_->arguments) {
        auto &llvmArg = *(it++);
        scoper_.getVariable(i++) = LocalVariable(false, &llvmArg);
        llvmArg.setName(utf8(arg.variableName));
    }
}

llvm::Value* FnCodeGenerator::sizeFor(llvm::PointerType *type) {
    auto one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 1);
    auto sizeg = builder().CreateGEP(llvm::ConstantPointerNull::getNullValue(type), one);
    return builder().CreatePtrToInt(sizeg, llvm::Type::getInt64Ty(generator()->context()));
}

llvm::Value* FnCodeGenerator::getMetaFromObject(llvm::Value *object) {
    std::vector<Value *> idx{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),  // object
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),  // classMeta
    };
    return builder().CreateLoad(builder().CreateGEP(object, idx), "meta");
}

llvm::Value* FnCodeGenerator::getHasBoxNoValue(llvm::Value *box) {
    auto null = llvm::Constant::getNullValue(typeHelper().valueTypeMetaPtr());
    return builder().CreateICmpEQ(builder().CreateLoad(getMetaTypePtr(box)), null);
}

Value* FnCodeGenerator::getMetaTypePtr(Value *box) {
    std::vector<Value *> idx{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
    };
    return builder().CreateGEP(box, idx);
}

Value* FnCodeGenerator::getHasNoValue(llvm::Value *simpleOptional) {
    auto vf = builder().CreateExtractValue(simpleOptional, 0);
    return builder().CreateICmpEQ(vf, generator()->optionalNoValue());
}

Value* FnCodeGenerator::getSimpleOptionalWithoutValue(const Type &type) {
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    return builder().CreateInsertValue(undef, generator()->optionalNoValue(), 0);
}

Value* FnCodeGenerator::getSimpleOptionalWithValue(llvm::Value *value, const Type &type) {
    auto structType = typeHelper().llvmTypeFor(type);
    auto undef = llvm::UndefValue::get(structType);
    auto simpleOptional = builder().CreateInsertValue(undef, value, 1);
    return builder().CreateInsertValue(simpleOptional, generator()->optionalValue(), 0);
}

Value* FnCodeGenerator::getValuePtr(Value *box, const Type &type) {
    std::vector<Value *> idx2{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 1),
    };
    auto llvmType = typeHelper().llvmTypeFor(type)->getPointerTo();
    return builder().CreateBitCast(builder().CreateGEP(box, idx2), llvmType);
}

Value* FnCodeGenerator::getObjectMetaPtr(Value *object) {
    auto metaIdx = std::vector<llvm::Value *> {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(generator()->context()), 0)
    };
    return builder().CreateGEP(object, metaIdx);
}

Value* FnCodeGenerator::getMakeNoValue(Value *box) {
    auto metaType = llvm::Constant::getNullValue(typeHelper().valueTypeMetaPtr());
    return builder().CreateStore(metaType, getMetaTypePtr(box));
}

void FnCodeGenerator::createIfElse(llvm::Value *cond, const std::function<void()> &then,
                                   const std::function<void()> &otherwise) {
    auto function = builder().GetInsertBlock()->getParent();
    auto success = llvm::BasicBlock::Create(generator()->context(), "then", function);
    auto fail = llvm::BasicBlock::Create(generator()->context(), "else", function);
    auto mergeBlock = llvm::BasicBlock::Create(generator()->context(), "cont", function);

    builder().CreateCondBr(cond, success, fail);

    builder().SetInsertPoint(success);
    then();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(fail);
    otherwise();
    builder().CreateBr(mergeBlock);

    builder().SetInsertPoint(mergeBlock);
}

llvm::Value* FnCodeGenerator::createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
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

}  // namespace EmojicodeCompiler
