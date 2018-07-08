//
//  FunctionType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionType_hpp
#define FunctionType_hpp

namespace EmojicodeCompiler {

class Function;

enum class FunctionType {
    ObjectMethod,
    ObjectInitializer,
    /** A function with a context. (e.g. a value type method) */
    ValueTypeMethod,
    ValueTypeInitializer,
    /** A type method. */
    ClassMethod,
    /** A plain function without a context. (🏁) */
    Function,
};

bool isSuperconstructorRequired(FunctionType);
bool isFullyInitializedCheckRequired(FunctionType);
bool isSelfAllowed(FunctionType);
bool hasInstanceScope(FunctionType);
bool isOnlyNothingnessReturnAllowed(FunctionType);
bool hasThisArgument(FunctionType);

}  // namespace EmojicodeCompiler

#endif /* FunctionType_hpp */
