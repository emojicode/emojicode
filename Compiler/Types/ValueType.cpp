//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Functions/Function.hpp"
#include "Compiler.hpp"
#include "Package/Package.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

ValueType::ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation, bool exported)
    : TypeDefinition(std::move(name), p, std::move(pos), documentation, exported) {}

Function* ValueType::copyRetain() {
    if (copyRetain_ == nullptr) {
        copyRetain_ = std::make_unique<Function>(U"copyretain", AccessLevel::Public, false, this, package(),
                                                    this->position(), false, U"", false, false, true, false,
                                                    FunctionType::CopyRetainer);
        copyRetain_->setReturnType(std::make_unique<ASTLiteralType>(Type::noReturn()));
    }
    return copyRetain_.get();
}

bool ValueType::isManaged() {
    if (managed_ == Managed::Unknown) {
        auto is = this == package()->compiler()->sMemory || (!isPrimitive() &&
                    std::any_of(instanceVariables().begin(), instanceVariables().end(), [](auto &decl) {
                        return decl.type->type().isManaged();
                    }));
        managed_ = is ? Managed::Yes : Managed::No;
    }
    return managed_ == Managed::Yes;
}

ValueType::~ValueType() = default;

}  // namespace EmojicodeCompiler
