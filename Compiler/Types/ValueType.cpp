//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"

namespace EmojicodeCompiler {

llvm::GlobalVariable* ValueType::valueTypeMetaFor(const std::vector<Type> &genericArguments) {
    auto it = valueTypeMetas_.find(genericArguments);
    if (it != valueTypeMetas_.end()) {
        return it->second;
    }
    return nullptr;
}

void ValueType::addValueTypeMetaFor(const std::vector<Type> &genericArguments, llvm::GlobalVariable *valueTypeMeta) {
    valueTypeMetas_.emplace(genericArguments, valueTypeMeta);
}

}  // namespace EmojicodeCompiler
