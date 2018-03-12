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
#include <llvm/IR/Function.h>

namespace EmojicodeCompiler {

llvm::FunctionType* FunctionReification::functionType() {
    if (functionType_ != nullptr) {
        return functionType_;
    }
    assert(function != nullptr);
    return function->getFunctionType();
}

Type Function::type() const {
    Type t = Type::callableIncomplete();
    t.genericArguments_.reserve(parameters().size() + 1);
    t.genericArguments_.push_back(returnType());
    for (auto &argument : parameters()) {
        t.genericArguments_.push_back(argument.type);
    }
    return t;
}

}  // namespace EmojicodeCompiler
