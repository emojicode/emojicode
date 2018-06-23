//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "Compiler.hpp"
#include "Types/TypeContext.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/IR/Function.h>

namespace EmojicodeCompiler {

llvm::FunctionType* FunctionReification::functionType() {
    if (functionType_ != nullptr) {
        return functionType_;
    }
    assert(function != nullptr);
    return function->getFunctionType();
}

TypeContext Function::typeContext() {
    auto type = owner_ == nullptr ? Type::noReturn() : owner_->type();
    if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
        type.setReference();
    }
    if (functionType() == FunctionType::ClassMethod) {
        type = Type(MakeTypeAsValue, type);
    }
    return TypeContext(type.applyMinimalBoxing(), this);
}

}  // namespace EmojicodeCompiler
