//
//  StringPool.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CodeGenerator.hpp"
#include "StringPool.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>

namespace EmojicodeCompiler {

llvm::Value* StringPool::pool(const std::u32string &string) {
    auto it = pool_.find(string);
    if (it != pool_.end()) {
        return it->second;
    }

    auto data = llvm::ArrayRef<uint32_t>(reinterpret_cast<const uint32_t *>(string.data()), string.size());
    auto constant = llvm::ConstantDataArray::get(codeGenerator_->context(), data);
    auto var = new llvm::GlobalVariable(*codeGenerator_->module(), constant->getType(), true,
                                        llvm::GlobalValue::LinkageTypes::InternalLinkage, constant);

    pool_.emplace(string, var);
    return var;
}

}  // namespace EmojicodeCompiler
