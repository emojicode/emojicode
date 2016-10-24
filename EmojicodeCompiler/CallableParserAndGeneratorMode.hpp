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
    ThisContextFunction,
    /** A type method. */
    ClassMethod,
    /** A plain function without a context. (🏁) */
    Function
};

#endif /* CallableParserAndGeneratorMode_h */
