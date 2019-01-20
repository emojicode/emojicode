//
//  StringPool.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "StringPool.hpp"
#include "CodeGenerator.hpp"
#include "Compiler.hpp"
#include "Generation/Declarator.hpp"
#include "Package/Package.hpp"
#include "Types/Class.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>

namespace EmojicodeCompiler {

llvm::Value* StringPool::pool(const std::u32string &string) {
    auto it = pool_.find(string);
    if (it != pool_.end()) {
        return it->second;
    }

    auto utf8str = utf8(string.data());
    auto data = llvm::ArrayRef<uint8_t>(reinterpret_cast<const uint8_t*>(utf8str.data()), utf8str.size());
    auto constant = llvm::ConstantStruct::getAnon({
        codeGenerator_->declarator().ignoreBlockPtr(),
        llvm::ConstantDataArray::get(codeGenerator_->context(), data)
    });
    auto var = new llvm::GlobalVariable(*codeGenerator_->module(), constant->getType(), true,
                                        llvm::GlobalValue::LinkageTypes::PrivateLinkage, constant);


    auto compiler = codeGenerator_->compiler();

    auto stringType = Type(compiler->sString);
    auto stringLlvm = llvm::dyn_cast<llvm::StructType>(llvm::dyn_cast<llvm::PointerType>(codeGenerator_->typeHelper().llvmTypeFor(stringType))->getElementType());

    auto stringStruct = llvm::ConstantStruct::get(stringLlvm, {
            codeGenerator_->declarator().ignoreBlockPtr(),
            compiler->sString->unspecificReification().classInfo,
            var,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(codeGenerator_->context()), utf8str.size())
    });

    auto stringVar = new llvm::GlobalVariable(*codeGenerator_->module(), stringLlvm, true,
                                              llvm::GlobalValue::LinkageTypes::PrivateLinkage, stringStruct, "string");

    pool_.emplace(string, stringVar);
    return stringVar;
}

}  // namespace EmojicodeCompiler
