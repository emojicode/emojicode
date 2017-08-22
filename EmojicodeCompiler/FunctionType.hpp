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
    /// A function which soley exists to unbox (generic) arguments passed to a protocol which the actual method does
    /// expect in another storage type. The function is then of type BoxingLayer.
    BoxingLayer,
};

bool isSuperconstructorRequired(FunctionType);
bool isFullyInitializedCheckRequired(FunctionType);
bool isSelfAllowed(FunctionType);
bool hasInstanceScope(FunctionType);
bool isOnlyNothingnessReturnAllowed(FunctionType);

}  // namespace Emojicode

#endif /* FunctionType_hpp */
