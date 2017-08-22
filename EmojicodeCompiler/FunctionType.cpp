//
//  FunctionType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "FunctionType.hpp"

namespace EmojicodeCompiler {

bool isSuperconstructorRequired(FunctionType type) {
    return type == FunctionType::ObjectInitializer;
}

bool isFullyInitializedCheckRequired(FunctionType type) {
    return type == FunctionType::ObjectInitializer || type == FunctionType::ValueTypeInitializer;
}

bool isSelfAllowed(FunctionType type) {
    return type != FunctionType::Function && type != FunctionType::ClassMethod;
}

bool hasInstanceScope(FunctionType type) {
    return (type == FunctionType::ObjectMethod || type == FunctionType::ObjectInitializer ||
            type == FunctionType::ValueTypeInitializer || type == FunctionType::ValueTypeMethod);
}

bool isOnlyNothingnessReturnAllowed(FunctionType type) {
    return type == FunctionType::ObjectInitializer || type == FunctionType::ValueTypeInitializer;
}

} // namespace EmojicodeCompiler
