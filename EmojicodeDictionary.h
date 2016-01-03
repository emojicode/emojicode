//
//  EmojicodeDictionary.h
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__EmojicodeDictionary__
#define __Emojicode__EmojicodeDictionary__

#include "EmojicodeString.h"

typedef struct {
    Object *key;
    Something value;
} EmojicodeDictionaryKVP;

typedef struct {
    Object *slots;
    size_t capacity;
    size_t count;
} EmojicodeDictionary;

/**
 * Insert an item and use keyString as key 
 * @warning GC-invoking
 */
void dictionarySet(Object *dicto, Object *keyString, Something value, Thread *thread);

/** Remove an item by keyString as key */
void dictionaryRemove(EmojicodeDictionary *dict, Object *keyString);

/** Get an item by keyString as key */
Something dictionaryLookup(EmojicodeDictionary *dict, Object *keyString);

void dictionaryMark(Object *dict);

void bridgeDictionaryInit(Thread *thread);

/** @warning GC-invoking */
void dictionaryInit(Thread *thread);

MethodHandler dictionaryMethodForName(EmojicodeChar name);

#endif /* defined(__Emojicode__EmojicodeDictionary__) */
