//
//  String.h
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeString_h
#define EmojicodeString_h

#include "EmojicodeAPI.hpp"

namespace Emojicode {

struct String {
    /// The number of characters. Strings are not null terminated.
    EmojicodeInteger length;
    /// The characters (Unicode Codepoints, @c EmojicodeChar) of this string.
    Object *charactersObject;

    EmojicodeChar* characters() { return charactersObject->val<EmojicodeChar>(); }
};

extern Object **stringPool;
#define emptyString (stringPool[0])

/** Compares if the value of @c a is equal to @c b. */
bool stringEqual(String *a, String *b);

/// Converts the string to a UTF8 char array and returns it.
/// @warning The returned pointer points into an object allocated by the Emojicode memory manager. It must not be free’d
/// and will not survive the imminent garbage collector cycle.
const char* stringToCString(Object *str);

/** Creates a string from a UTF8 C string. The string must be null terminated! */
Object* stringFromChar(const char *cstring);

void stringMark(Object *self);

struct List;

void initStringFromSymbolList(String *string, List *list);

void stringPrintStdoutBrigde(Thread *thread);
void stringEqualBridge(Thread *thread);
void stringSubstringBridge(Thread *thread);
void stringIndexOf(Thread *thread);
void stringTrimBridge(Thread *thread);
void stringGetInput(Thread *thread);
void stringSplitByStringBridge(Thread *thread);
void stringLengthBridge(Thread *thread);
void stringUTF8LengthBridge(Thread *thread);
void stringByAppendingSymbolBridge(Thread *thread);
void stringSymbolAtBridge(Thread *thread);
void stringBeginsWithBridge(Thread *thread);
void stringEndsWithBridge(Thread *thread);
void stringSplitBySymbolBridge(Thread *thread);
void stringToData(Thread *thread);
void stringToCharacterList(Thread *thread);
void stringJSON(Thread *thread);
void stringFromSymbolListBridge(Thread *thread);
void stringFromStringList(Thread *thread);
void stringToInteger(Thread *thread);
void stringToDouble(Thread *thread);
void stringToUppercase(Thread *thread);
void stringToLowercase(Thread *thread);
void stringCompareBridge(Thread *thread);

}

#endif /* EmojicodeString_h */
