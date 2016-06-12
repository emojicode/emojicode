//
//  String.h
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeString_h
#define EmojicodeString_h

#include "EmojicodeAPI.h"
#include "EmojicodeList.h"

extern Object **stringPool;
#define emptyString (stringPool[0])
#define characters(string) ((EmojicodeChar*)(string)->characters->value)

/** Compares if the value of @c a is equal to @c b. */
bool stringEqual(String *a, String *b);

/**
 * Converts the string to a UTF8 char array and returns it.
 * @warning You must take care of releasing the allocated memory by calling @c free.
 */
char* stringToChar(String *str);

/** Creates a string from a UTF8 C string. The string must be null terminated! */
Object* stringFromChar(const char *cstring);

/** 
 * Tries to parse the string in the this-slot on the stack as JSON.
 * @returns The top-level object or Nothingness if there was an error parsing the string.
 */
Something parseJSON(Thread *thread);

void stringMark(Object *self);

void initStringFromSymbolList(Object *string, List *list);

FunctionFunctionPointer stringMethodForName(EmojicodeChar name);
InitializerFunctionFunctionPointer stringInitializerForName(EmojicodeChar name);

#endif /* EmojicodeString_h */
