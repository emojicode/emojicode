//
//  EmojicodeDictionary.h
//  Emojicode
//
//  Created by Theo Weidmann on 19/04/15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeDictionary_h
#define EmojicodeDictionary_h

#include "String.h"

namespace Emojicode {

/** Default initial capacity. MUST be a power of two, default: 8 */
#define DICTIONARY_DEFAULT_INITIAL_CAPACITY (1 << 3)
/** Factor for determining whether the dictionary should be resized. */
#define DICTIONARY_DEFAULT_LOAD_FACTOR (0.75f)
#define DICTIONARY_MAXIMUM_CAPACTIY (1 << 30)
#define DICTIONARY_MAXIMUM_CAPACTIY_THRESHOLD (1 << 16)

typedef uint64_t EmojicodeDictionaryHash;

/** The datastructure with the key-value pair */
struct EmojicodeDictionaryNode {
    /** The user specified key. */
    Object *key;

    /** The user specified value. */
    Box value;

    /** The cached hash for the key. Calculated on item addition. */
    EmojicodeDictionaryHash hash;

    /** EmojicodeDictionary stores a bucket's elements in a linked list. */
    Object *next;
};

/** Structure for the Emojicode standard Dictionary. The implementation is similar to Java's Hashmap. */
struct EmojicodeDictionary {
    /** An array with pointers to linked lists. Initializes when the first item is inserted. */
    Object *buckets;

    /** Length of buckets array. */
    size_t bucketsCounter;

    /** The number of items stored in this dictionary. Used for determining whether to resize. */
    size_t size;

    /** When size * loadFactor >= bucketsCounter the dictionary doubles the amount of buckets */
    float loadFactor;

    /** Stores the next threshold for resizing. Is 0 until first resize when item is inserted. */
    size_t nextThreshold;
};

/// Prepares the dictionary for a new value and returns a pointer to where the new value should be copied.
/// @warning Garbage collector invoking
Box* dictionaryPutVal(RetainedObjectPointer dictionaryObject, RetainedObjectPointer key, Thread *thread);
/// Removes the key and the associated value from the dictionary, if the key @c key is in the dictionary.
void dictionaryRemove(EmojicodeDictionary *dictionary, Object *key);


void dictionaryMark(Object *dict);

void initDictionaryBridge(Thread *thread);
void dictionaryInit(EmojicodeDictionary *dict);

void bridgeDictionarySet(Thread *thread);
void bridgeDictionaryGet(Thread *thread);
void bridgeDictionaryRemove(Thread *thread);
void bridgeDictionaryKeys(Thread *thread);
void bridgeDictionaryClear(Thread *thread);
void bridgeDictionaryContains(Thread *thread);
void bridgeDictionarySize(Thread *thread);

}

#endif /* EmojicodeDictionary_h */
