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
#include <utility>

namespace Emojicode {

void expandListSize(Object *const &listObject) {
#define initialSize 7
    List *list = static_cast<List *>(listObject->value);
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Box) * initialSize);
        list = static_cast<List *>(listObject->value);
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = resizeArray(list->items, sizeCalculationWithOverflowProtection(newSize, sizeof(Box)));
        list = static_cast<List *>(listObject->value);
        list->items = object;
        list->capacity = newSize;
    }
#undef initialSize
}

void listEnsureCapacity(Thread *thread, size_t size) {
    List *list = static_cast<List *>(thread->getThisObject()->value);
    if (list->capacity < size) {
        Object *object;
        if (list->capacity == 0) {
            object = newArray(sizeCalculationWithOverflowProtection(size, sizeof(Box)));
        }
        else {
            object = resizeArray(list->items, sizeCalculationWithOverflowProtection(size, sizeof(Box)));
        }
        list = static_cast<List *>(thread->getThisObject()->value);
        list->items = object;
        list->capacity = size;
    }
}

void listMark(Object *self) {
    List *list = static_cast<List *>(self->value);
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
    List *list = static_cast<List *>(lo->value);
    if (list->capacity - list->count == 0) {
        expandListSize(listObject);
    }
    list = static_cast<List *>(listObject->value);
    thread->release(1);
    return list->elements() + list->count++;
}

void listCountBridge(Thread *thread, Value *destination) {
    destination->raw = static_cast<List *>(thread->getThisObject()->value)->count;
}

void listAppendBridge(Thread *thread, Value *destination) {
    *listAppendDestination(thread->getThisObject(), thread) = *reinterpret_cast<Box *>(thread->variableDestination(0));
}

void listGetBridge(Thread *thread, Value *destination) {
    List *list = static_cast<List *>(thread->getThisObject()->value);
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
    List *list = static_cast<List *>(thread->getThisObject()->value);
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
    List *list = static_cast<List *>(thread->getThisObject()->value);
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
    List *list = static_cast<List *>(thread->getThisObject()->value);

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

    list = static_cast<List *>(thread->getThisObject()->value);

    std::memmove(list->elements() + index + 1, list->elements() + index, sizeof(Box) * (list->count++ - index));
    list->elements()[index].copy(thread->variableDestination(1));
}

void listQSort(Thread *thread, size_t off, size_t n) {
    if (n < 2)
        return;

    Box *items = ((List *)thread->getThisObject()->value)->elements() + off;
    Box pivot = items[n / 2];
    size_t i, j;

    for (i = 0, j = n - 1; ; i++, j--) {
        while (true) {
            Value args[2 * STORAGE_BOX_VALUE_SIZE];
            items[i].copyTo(args);
            pivot.copyTo(args + STORAGE_BOX_VALUE_SIZE);
            Value c;
            executeCallableExtern(thread->getVariable(0).object, args, thread, &c);
            items = ((List *)thread->getThisObject()->value)->elements() + off;
            if (c.raw >= 0) break;
            i++;
        }

        while (true) {
            Value args[2 * STORAGE_BOX_VALUE_SIZE];
            items[i].copyTo(args);
            pivot.copyTo(args + STORAGE_BOX_VALUE_SIZE);
            Value c;
            executeCallableExtern(thread->getVariable(0).object, args, thread, &c);
            items = ((List *)thread->getThisObject()->value)->elements() + off;
            if (c.raw >= 0) break;
            j--;
        }

        if (i >= j)
            break;

        std::swap(items[i], items[j]);
    }

    listQSort(thread, off, i);
    listQSort(thread, off + i, n - i);
}

void listSort(Thread *thread, Value *destination) {
    List *list = static_cast<List *>(thread->getThisObject()->value);
    listQSort(thread, 0, list->count);
}

void listFromListBridge(Thread *thread, Value *destination) {
    Object *const &listO = thread->retain(newObject(CL_LIST));

    List *list = static_cast<List *>(listO->value);
    List *originalList = static_cast<List *>(thread->getThisObject()->value);

    list->count = originalList->count;
    list->capacity = originalList->capacity;

    Object *items = newArray(sizeof(Box) * originalList->capacity);
    list = static_cast<List *>(listO->value);
    originalList = static_cast<List *>(thread->getThisObject()->value);
    list->items = items;

    std::memcpy(list->elements(), originalList->elements(), originalList->count * sizeof(Box));
    thread->release(1);
    destination->object = listO;
}

void listRemoveAllBridge(Thread *thread, Value *destination) {
    List *list = static_cast<List *>(thread->getThisObject()->value);
    std::memset(list->elements(), 0, list->count * sizeof(Box));
    list->count = 0;
}

void listSetBridge(Thread *thread, Value *destination) {
    EmojicodeInteger index = thread->getVariable(0).raw;
    List *list = static_cast<List *>(thread->getThisObject()->value);

    listEnsureCapacity(thread, index + 1);
    list = static_cast<List *>(thread->getThisObject()->value);

    if (list->count <= index)
        list->count = index + 1;

    list->elements()[index].copy(thread->variableDestination(1));
}

void listShuffleInPlaceBridge(Thread *thread, Value *destination) {
    List *list = static_cast<List *>(thread->getThisObject()->value);
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
    List *list = static_cast<List *>(thread->getThisObject()->value);
    list->capacity = capacity;
    list->items = n;
    *destination = thread->getThisContext();
}

}
