//
//  Function.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Function.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "EmojicodeCompiler.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/IR/Function.h>
#include <algorithm>
#include <stdexcept>

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
    t.genericArguments_.reserve(arguments().size() + 1);
    t.genericArguments_.push_back(returnType());
    for (auto &argument : arguments()) {
        t.genericArguments_.push_back(argument.type);
    }
    return t;
}

}  // namespace EmojicodeCompiler
