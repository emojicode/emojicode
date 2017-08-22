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
#include "RetainedObjectPointer.hpp"

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
Box* listAppendDestination(RetainedObjectPointer listObject, Thread *thread);

void listMark(Object *self);

void initListEmptyBridge(Thread *thread);
void initListWithCapacity(Thread *thread);
void listCountBridge(Thread *thread);
void listAppendBridge(Thread *thread);
void listGetBridge(Thread *thread);
void listRemoveBridge(Thread *thread);
void listPopBridge(Thread *thread);
void listInsertBridge(Thread *thread);
void listSort(Thread *thread);
void listFromListBridge(Thread *thread);
void listRemoveAllBridge(Thread *thread);
void listSetBridge(Thread *thread);
void listShuffleInPlaceBridge(Thread *thread);
void listEnsureCapacityBridge(Thread *thread);
void listAppendList(Thread *thread);

}

#endif /* EmojicodeList_h */
