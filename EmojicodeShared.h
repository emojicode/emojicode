//
//  EmojicodeSharedTypes.h
//  Emojicode
//
//  Created by Theo Weidmann on 19/07/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeShared_h
#define EmojicodeShared_h

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

/** An Unicode codepoint */
typedef uint32_t EmojicodeChar;

typedef int_fast64_t EmojicodeInteger;
/** The bytecode file is structured into Coins. Each Coin represents a single instruction or value. */
typedef uint32_t EmojicodeCoin;

/* Using either of them in a package makes absolutely no sense */
#ifndef defaultPackagesDirectory
#define defaultPackagesDirectory "/usr/local/EmojicodePackages"
#endif
extern const char *packageDirectory;
#define ByteCodeSpecificationVersion 5

/**
 * @defined(isWhitespace)
 * @discussion Inserts a test if @c c is a whitespace
 * http://www.unicode.org/Public/6.3.0/ucd/PropList.txt
 * @param c The character to test
 */
#define isWhitespace(c) ((0x9 <= c && c <= 0xD) || c == 0x20 || c == 0x85 || c == 0xA0 || c == 0x1680 || (0x2000 <= c && c <= 0x200A) || c == 0x2028 || c== 0x2029 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000 || c == 0xFE0F)

#define ecCharToCharStack(ec, outVariable)\
char outVariable[5] = {0, 0, 0, 0, 0};\
u8_wc_toutf8(outVariable, (ec));

#define PORTABLE_INTLEAST64_MAX ((int_least64_t)9223372036854775807)

#endif /* EmojicodeShared_h */
