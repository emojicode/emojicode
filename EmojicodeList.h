//
//  List.h
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeList_h
#define EmojicodeList_h

#include "EmojicodeAPI.h"

struct List {
    /** The number of elements in the list. */
    size_t count;
    /** The capacity of the list. */
    size_t capacity;
    /**
     * The array which stores the list items which has a size of @c capacity * sizeof(Something).
     * Can be @c NULL if @c capacity is 0.
     */
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
 * Returns the item at @c i or @c NULL if @c i is out of bounds.
 */
Something listGet(List *list, EmojicodeInteger i);

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

FunctionFunctionPointer listMethodForName(EmojicodeChar method);
InitializerFunctionFunctionPointer listInitializerForName(EmojicodeChar method);

#endif /* EmojicodeList_h */
