//
//  CallableParserAndGeneratorMode.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/09/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableParserAndGeneratorMode_h
#define CallableParserAndGeneratorMode_h

enum class CallableParserAndGeneratorMode {
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

#endif /* CallableParserAndGeneratorMode_h */
