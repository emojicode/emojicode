//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Compiler.hpp"
#include "Package/Package.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

ValueType::ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation,
                     bool exported, bool primitive)
    : TypeDefinition(std::move(name), p, std::move(pos), documentation, exported), primitive_(primitive) {}

bool ValueType::isManaged() {
    if (managed_ == Managed::Unknown) {
        auto is = storesGenericArgs() || this == package()->compiler()->sMemory || (!isPrimitive() &&
                    std::any_of(instanceVariables().begin(), instanceVariables().end(), [](auto &decl) {
                        return decl.type->type().isManaged();
                    }));
        managed_ = is ? Managed::Yes : Managed::No;
    }
    return managed_ == Managed::Yes;
}

bool ValueType::storesGenericArgs() const {
    if (isGenericDynamismDisabled()) return false;
    return !genericParameters().empty();
}

ValueType::~ValueType() = default;

}  // namespace EmojicodeCompiler
