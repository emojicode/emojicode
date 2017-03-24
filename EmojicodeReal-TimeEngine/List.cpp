//
//  List.c
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "List.h"
#include "String.h"
#include "Thread.hpp"
#include "standard.h"
#include <cstring>
#include <algorithm>
#include <utility>

namespace Emojicode {

void expandListSize(Object *const &listObject) {
#define initialSize 7
    List *list = listObject->val<List>();
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Box) * initialSize);
        list = listObject->val<List>();
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = resizeArray(list->items, sizeCalculationWithOverflowProtection(newSize, sizeof(Box)));
        list = listObject->val<List>();
        list->items = object;
        list->capacity = newSize;
    }
#undef initialSize
}

void listEnsureCapacity(Thread *thread, size_t size) {
    List *list = thread->getThisObject()->val<List>();
    if (list->capacity < size) {
        Object *object;
        if (list->capacity == 0) {
            object = newArray(sizeCalculationWithOverflowProtection(size, sizeof(Box)));
        }
        else {
            object = resizeArray(list->items, sizeCalculationWithOverflowProtection(size, sizeof(Box)));
        }
        list = thread->getThisObject()->val<List>();
        list->items = object;
        list->capacity = size;
    }
}

void listMark(Object *self) {
    List *list = self->val<List>();
    if (list->items) {
        mark(&list->items);
    }
    for (size_t i = 0; i < list->count; i++) {
        if (list->elements()[i].type.raw == T_OBJECT) {
           mark(&list->elements()[i].value1.object);
        }
    }
}

Box* listAppendDestination(Object *lo, Thread *thread) {
    Object *const &listObject = thread->retain(lo);
    List *list = lo->val<List>();
    if (list->capacity - list->count == 0) {
        expandListSize(listObject);
    }
    list = listObject->val<List>();
    thread->release(1);
    return list->elements() + list->count++;
}

void listCountBridge(Thread *thread, Value *destination) {
    destination->raw = thread->getThisObject()->val<List>()->count;
}

void listAppendBridge(Thread *thread, Value *destination) {
    *listAppendDestination(thread->getThisObject(), thread) = *reinterpret_cast<Box *>(thread->variableDestination(0));
}

void listGetBridge(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    EmojicodeInteger index = thread->getVariable(0).raw;
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        destination->makeNothingness();
        return;
    }
    list->elements()[index].copyTo(destination);
}

void listRemoveBridge(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    EmojicodeInteger index = thread->getVariable(0).raw;
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        destination->raw = 0;
        return;
    }
    std::memmove(list->elements() + index, list->elements() + index + 1, sizeof(Box) * (list->count - index - 1));
    list->elements()[--list->count].makeNothingness();
    destination->raw = 1;
}

void listPopBridge(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    if (list->count == 0) {
        destination->makeNothingness();
        return;
    }
    size_t index = --list->count;
    Box v = list->elements()[index];
    list->elements()[index].makeNothingness();
    v.copyTo(destination);
}

void listInsertBridge(Thread *thread, Value *destination) {
    EmojicodeInteger index = thread->getVariable(0).raw;
    List *list = thread->getThisObject()->val<List>();

    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        destination->makeNothingness();
        return;
    }

    if (list->capacity - list->count == 0) {
        Object *&object = thread->currentStackFrame()->thisContext.object;
        expandListSize(object);
    }

    list = thread->getThisObject()->val<List>();

    std::memmove(list->elements() + index + 1, list->elements() + index, sizeof(Box) * (list->count++ - index));
    list->elements()[index].copy(thread->variableDestination(1));
}

void listSort(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    std::sort(list->elements(), list->elements() + list->count, [thread](const Box &a, const Box &b) {
        Value args[2 * STORAGE_BOX_VALUE_SIZE];
        a.copyTo(args);
        b.copyTo(args + STORAGE_BOX_VALUE_SIZE);
        Value c;
        executeCallableExtern(thread->getVariable(0).object, args, sizeof(args), thread, &c);
        return c.raw < 0;
    });
}

void listFromListBridge(Thread *thread, Value *destination) {
    Object *const &listO = thread->retain(newObject(CL_LIST));

    List *list = listO->val<List>();
    List *originalList = thread->getThisObject()->val<List>();

    list->count = originalList->count;
    list->capacity = originalList->capacity;

    Object *items = newArray(sizeof(Box) * originalList->capacity);
    list = listO->val<List>();
    originalList = thread->getThisObject()->val<List>();
    list->items = items;

    std::memcpy(list->elements(), originalList->elements(), originalList->count * sizeof(Box));
    thread->release(1);
    destination->object = listO;
}

void listRemoveAllBridge(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    std::memset(list->elements(), 0, list->count * sizeof(Box));
    list->count = 0;
}

void listSetBridge(Thread *thread, Value *destination) {
    EmojicodeInteger index = thread->getVariable(0).raw;
    List *list = thread->getThisObject()->val<List>();

    listEnsureCapacity(thread, index + 1);
    list = thread->getThisObject()->val<List>();

    if (list->count <= index)
        list->count = index + 1;

    list->elements()[index].copy(thread->variableDestination(1));
}

void listShuffleInPlaceBridge(Thread *thread, Value *destination) {
    List *list = thread->getThisObject()->val<List>();
    EmojicodeInteger i, n = (EmojicodeInteger)list->count;
    Box tmp;

    for (i = n - 1; i > 0; i--) {
        EmojicodeInteger newIndex = secureRandomNumber(0, i);
        std::swap(list->elements()[i], list->elements()[newIndex]);
    }
}

void listEnsureCapacityBridge(Thread *thread, Value *destination) {
    listEnsureCapacity(thread, thread->getVariable(0).raw);
}

void initListEmptyBridge(Thread *thread, Value *destination) {
    // The Real-Time Engine guarantees pre-nulled objects.
    *destination = thread->getThisContext();
}

void initListWithCapacity(Thread *thread, Value *destination) {
    EmojicodeInteger capacity = thread->getVariable(0).raw;
    Object *n = newArray(sizeCalculationWithOverflowProtection(capacity, sizeof(Box)));
    List *list = thread->getThisObject()->val<List>();
    list->capacity = capacity;
    list->items = n;
    *destination = thread->getThisContext();
}

}
