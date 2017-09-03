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
#include "../AST/ASTStatements.hpp"
#include "../Functions/BoxingLayer.hpp"
#include "FnCodeGenerator.hpp"
#include <cstdlib>

namespace EmojicodeCompiler {

void FnCodeGenerator::generate() {
    if (fn_->isNative()) {
        wr().writeInstruction({ INS_TRANSFER_CONTROL_TO_NATIVE, INS_RETURN });
        return;
    }

    llvm::StructType::get(<#llvm::LLVMContext &Context#>, <#ArrayRef<llvm::Type *> Elements#>)

    std::vector<llvm::Type *> args(3, llvm::Type::getDoubleTy(app()->context()));
    auto ft = llvm::FunctionType::get(llvm::Type::getInt64Ty(app()->context()), args, false);
    auto function = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "tests3", app()->module());

    auto basicBlock = llvm::BasicBlock::Create(app()->context(), "entry", function);
    builder_.SetInsertPoint(basicBlock);

    scoper_.pushScope();

    declareArguments();

    fn_->ast()->generate(this);

    fn_->setFullSize(scoper_.size());
    scoper_.popScope(wr().count());
}

void FnCodeGenerator::declareArguments() {
    unsigned int i = 0;
    for (auto arg : fn_->arguments) {
        scoper_.declareVariable(VariableID(i++), arg.type).initialize(0);
    }
}

void FnCodeGenerator::writeInteger(long long value)  {
    if (std::llabs(value) > INT32_MAX) {
        wr().writeInstruction(INS_GET_64_INTEGER);

        wr().writeInstruction(value >> 32);
        wr().writeInstruction(static_cast<EmojicodeInstruction>(value));
        return;
    }

    wr().writeInstruction(INS_GET_32_INTEGER);
    value += INT32_MAX;
    wr().writeInstruction(static_cast<EmojicodeInstruction>(value));
}

}  // namespace EmojicodeCompiler
