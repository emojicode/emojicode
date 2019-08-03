//
//  FunctionType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "FunctionType.hpp"
#include "Function.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

bool isSuperconstructorRequired(FunctionType type) {
    return type == FunctionType::ObjectInitializer;
}

bool isFullyInitializedCheckRequired(FunctionType type) {
    return type == FunctionType::ObjectInitializer || type == FunctionType::ValueTypeInitializer;
}

bool isSelfAllowed(FunctionType type) {
    return type != FunctionType::Function;
}

bool hasInstanceScope(FunctionType type) {
    return type == FunctionType::ObjectMethod || type == FunctionType::ObjectInitializer ||
           type == FunctionType::ValueTypeInitializer || type == FunctionType::ValueTypeMethod ||
           type == FunctionType::Deinitializer;
}

bool isReturnForbidden(FunctionType type) {
    return type == FunctionType::ObjectInitializer || type == FunctionType::ValueTypeInitializer;
}

bool hasThisArgument(Function *function) {
    return isSelfAllowed(function->functionType()) &&
            !(function->functionType() == FunctionType::ObjectInitializer &&
              dynamic_cast<Class *>(function->owner())->foreign());
}

bool isTypeMethod(Function *function) {
    return function->functionType() == FunctionType::ClassMethod ||
            (function->functionType() == FunctionType::Function && function->owner() != nullptr);
}

} // namespace EmojicodeCompiler
