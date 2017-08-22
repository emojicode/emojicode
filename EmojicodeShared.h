//
//  EmojicodeSharedTypes.h
//  Emojicode
//
//  Created by Theo Weidmann on 19/07/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeShared_h
#define EmojicodeShared_h

#include <cstdint>

/// A Unicode codepoint
typedef uint32_t EmojicodeChar;

/// A byte-code instruction
typedef uint32_t EmojicodeInstruction;

#ifndef defaultPackagesDirectory
#define defaultPackagesDirectory "/usr/local/EmojicodePackages"
#endif

#define T_NOTHINGNESS static_cast<EmojicodeInteger>(0)
#define T_OBJECT 1
#define T_OPTIONAL_VALUE static_cast<EmojicodeInteger>(1)
#define T_ERROR static_cast<EmojicodeInteger>(2)

#define T_BOOLEAN 3
#define T_INTEGER 4
#define T_DOUBLE 5
#define T_SYMBOL 6

#define REMOTE_MASK (1 << 31)

/**
 * @defined(isWhitespace)
 * @discussion Inserts a test if @c c is a whitespace
 * http://www.unicode.org/Public/6.3.0/ucd/PropList.txt
 * @param c The character to test
 */
#define isWhitespace(c) ((0x9 <= c && c <= 0xD) || c == 0x20 || c == 0x85 || c == 0xA0 || c == 0x1680 || (0x2000 <= c && c <= 0x200A) || c == 0x2028 || c== 0x2029 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000 || c == 0xFE0F)

#define PORTABLE_INTLEAST64_MAX ((int_least64_t)9223372036854775807)

enum class ObjectVariableType {
    /// There is an object pointer a the given index
    Simple = 0,
    /// There is an object pointer a the given index if the value at @c condition is truthy
    Condition = 1,
    Box = 2,
    ConditionalSkip = 3
};

enum class ContextType {
    None = 0,
    Object = 1,
    ValueReference = 2,
};

#endif /* EmojicodeShared_h */
