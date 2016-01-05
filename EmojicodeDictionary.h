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

// MUST be a power of two, default: 8
#define DICTIONARY_DEFAULT_INITIAL_CAPACITY (1 << 3)
// Factor for determining whether the dictionary should be resized
#define DICTIONARY_DEFAULT_LOAD_FACTOR (0.75f)

#define DICTIONARY_MAXIMUM_CAPACTIY (1 << 30)

#define DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD (1 << 16)

typedef uint64_t EmojicodeDictionaryHash;

typedef struct {
    Something key;
    Something value;
    EmojicodeDictionaryHash hash;
    Object *next;
} EmojicodeDictionaryNode;

typedef struct {
    Object *table;
    size_t buckets;
    size_t size;
    float loadFactor;
    size_t nextThreshold;
} EmojicodeDictionary;

/**
 * Insert an item and use keyString as key 
 * @warning GC-invoking
 */
void dictionarySet(Object *dicto, Something key, Something value, Thread *thread);

/** Remove an item by keyString as key */
void dictionaryRemove(EmojicodeDictionary *dict, Something key, Thread *thread);

/** Get an item by keyString as key */
Something dictionaryLookup(EmojicodeDictionary *dict, Something key, Thread *thread);

void dictionaryMark(Object *dict);

void bridgeDictionaryInit(Thread *thread);

/** @warning GC-invoking */
void dictionaryInit(Thread *thread);

MethodHandler dictionaryMethodForName(EmojicodeChar name);

#endif /* defined(__Emojicode__EmojicodeDictionary__) */
