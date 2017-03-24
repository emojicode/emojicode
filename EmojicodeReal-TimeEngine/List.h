//
//  List.h
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeList_h
#define EmojicodeList_h

#include "EmojicodeAPI.hpp"

namespace Emojicode {

struct List {
    /** The number of elements in the list. */
    size_t count;
    /** The capacity of the list. */
    size_t capacity;
    /**
     * The array which stores the list items which has a size of @c capacity * sizeof(Value).
     * Can be @c nullptr if @c capacity is 0.
     */
    Object *items;

    Box* elements() const { return items->val<Box>(); }
};

/// Prepares the list for a new element to be added to the end and returns a pointer to where the new element should be
/// copied.
Box* listAppendDestination(Object *lo, Thread *thread);

/**
 * Creates a list by copying all references from @c cpdList.
 */
Object* listFromList(List *cpdList);

/**
 * Shuffles the list in place by using the Fisher Yates alogrithm.
 */
void listShuffleInPlace(List *list);

/** Releases list @c l */
void listRelease(void *l);

/** List marker for the GC */
void listMark(Object *self);

void initListEmptyBridge(Thread *thread, Value *destination);
void initListWithCapacity(Thread *thread, Value *destination);
void listCountBridge(Thread *thread, Value *destination);
void listAppendBridge(Thread *thread, Value *destination);
void listGetBridge(Thread *thread, Value *destination);
void listRemoveBridge(Thread *thread, Value *destination);
void listPopBridge(Thread *thread, Value *destination);
void listInsertBridge(Thread *thread, Value *destination);
void listSort(Thread *thread, Value *destination);
void listFromListBridge(Thread *thread, Value *destination);
void listRemoveAllBridge(Thread *thread, Value *destination);
void listSetBridge(Thread *thread, Value *destination);
void listShuffleInPlaceBridge(Thread *thread, Value *destination);
void listEnsureCapacityBridge(Thread *thread, Value *destination);

}

#endif /* EmojicodeList_h */
