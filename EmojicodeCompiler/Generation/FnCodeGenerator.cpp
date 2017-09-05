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

}  // namespace EmojicodeCompiler
