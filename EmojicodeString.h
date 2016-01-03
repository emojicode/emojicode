//
//  String.h
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__String__
#define __Emojicode__String__

#include "EmojicodeAPI.h"
#include "EmojicodeList.h"

extern Object **stringPool;
#define emptyString (stringPool[0])
#define characters(string) ((EmojicodeChar*)(string)->characters->value)

/** Comparse if the value of a is equal to b */
bool stringEqual(String *a, String *b);

/** Converts the string to a UTF8 char array. @warning Do not forget to free the char array. */
char* stringToChar(String *str);

/** Creates a string from a UTF8 C string. The string must be null terminated! */
Object* stringFromChar(const char *cstring);

void stringMark(Object *self);

void initStringFromSymbolList(Object *string, List *list);

MethodHandler stringMethodForName(EmojicodeChar name);
InitializerHandler stringInitializerForName(EmojicodeChar name);

Something parseJSON(Thread *thread);

#endif /* defined(__Emojicode__String__) */
