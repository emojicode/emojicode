//
//  List.h
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__List__
#define __Emojicode__List__

#include "EmojicodeAPI.h"

struct List {
    size_t count;
    size_t capacity;
    Object *items;
};

/** 
 * Inserts @c o at the end of the list. O(1) 
 * @warning GC-invoking
 */
void listAppend(Object *lo, Something o, Thread *thread);

/** Removes the last element of the list and returns it. O(1) */
Something listPop(List *list);

/**
 * Removes the element at @c index. O(n - index)
 * @return @c false if index is out of bounds, otherwise @c true.
 */
bool listRemoveByIndex(List *list, size_t index);

/**
 * Returns the item at @c i or @c NULL if @c i is out of bounds.
 */
Something listGet(List *list, size_t i);

/**
 * Creates a list by copying all references from @c cpdList.
 */
Object* listFromList(List *cpdList);

/**
 * Removes the first occourence of x.
 */
bool listRemove(List *list, Something x);

/**
 * Shuffles the list in place by using the Fisher Yates alogrithm.
 */
void listShuffleInPlace(List *list);

/** Releases list @c l */
void listRelease(void *l);

/** List marker for the GC */
void listMark(Object *self);

MethodHandler listMethodForName(EmojicodeChar method);
InitializerHandler listInitializerForName(EmojicodeChar method);

#endif /* defined(__Emojicode__List__) */
