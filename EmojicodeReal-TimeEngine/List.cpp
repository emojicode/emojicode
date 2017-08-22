//
//  List.c
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "List.hpp"
#include "String.hpp"
#include "Thread.hpp"
#include "Memory.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <utility>

namespace Emojicode {

void expandListSize(RetainedObjectPointer listObject, Thread *thread) {
#define initialSize 7
    auto *list = listObject->val<List>();
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Box) * initialSize);
        list = listObject->val<List>();
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = resizeArray(list->items, sizeCalculationWithOverflowProtection(newSize, sizeof(Box)), thread);
        list = listObject->val<List>();
        list->items = object;
        list->capacity = newSize;
    }
#undef initialSize
}

void listEnsureCapacity(Thread *thread, size_t size) {
    auto *list = thread->thisObject()->val<List>();
    if (list->capacity < size) {
        Object *object;
        if (list->capacity == 0) {
            object = newArray(sizeCalculationWithOverflowProtection(size, sizeof(Box)));
        }
        else {
            object = resizeArray(list->items, sizeCalculationWithOverflowProtection(size, sizeof(Box)), thread);
        }
        list = thread->thisObject()->val<List>();
        list->items = object;
        list->capacity = size;
    }
}

void listMark(Object *self) {
    auto *list = self->val<List>();
    if (list->items) {
        mark(&list->items);
    }
    for (size_t i = 0; i < list->count; i++) {
        markBox(&list->elements()[i]);
    }
}

Box* listAppendDestination(RetainedObjectPointer listObject, Thread *thread) {
    auto *list = listObject->val<List>();
    if (list->capacity - list->count == 0) {
        expandListSize(listObject, thread);
    }
    list = listObject->val<List>();
    return list->elements() + list->count++;
}

void listAppendList(Thread *thread) {
    auto *copyList = thread->variable(0).object->val<List>();
    listEnsureCapacity(thread, thread->thisObject()->val<List>()->count + copyList->count);
    auto *list = thread->thisObject()->val<List>();
    copyList = thread->variable(0).object->val<List>();
    std::memcpy(list->elements() + list->count, copyList->elements(), copyList->count * sizeof(Box));
    list->count += copyList->count;
}

void listCountBridge(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(thread->thisObject()->val<List>()->count));
}

void listAppendBridge(Thread *thread) {
    *listAppendDestination(thread->thisObjectAsRetained(),
                           thread) = *reinterpret_cast<Box *>(thread->variableDestination(0));
    thread->returnFromFunction();
}

void listGetBridge(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();
    EmojicodeInteger index = thread->variable(0).raw;
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        thread->returnNothingnessFromFunction();
        return;
    }
    thread->returnFromFunction(list->elements()[index]);
}

void listRemoveBridge(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();
    EmojicodeInteger index = thread->variable(0).raw;
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        thread->returnFromFunction(false);
        return;
    }
    std::memmove(list->elements() + index, list->elements() + index + 1, sizeof(Box) * (list->count - index - 1));
    list->elements()[--list->count].makeNothingness();
    thread->returnFromFunction(true);
}

void listPopBridge(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();
    if (list->count == 0) {
        thread->returnNothingnessFromFunction();
        return;
    }
    size_t index = --list->count;
    Box v = list->elements()[index];
    list->elements()[index].makeNothingness();
    thread->returnFromFunction(v);
}

void listInsertBridge(Thread *thread) {
    EmojicodeInteger index = thread->variable(0).raw;
    auto *list = thread->thisObject()->val<List>();

    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index) {
        thread->returnFromFunction();
        return;
    }

    if (list->capacity - list->count == 0) {
        expandListSize(thread->thisObjectAsRetained(), thread);
    }

    list = thread->thisObject()->val<List>();

    std::memmove(list->elements() + index + 1, list->elements() + index, sizeof(Box) * (list->count++ - index));
    list->elements()[index].copy(thread->variableDestination(1));
    thread->returnFromFunction();
}

void listSort(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();
    std::sort(list->elements(), list->elements() + list->count, [thread](const Box &a, const Box &b) {
        Value args[2 * kBoxValueSize];
        a.copyTo(args);
        b.copyTo(args + kBoxValueSize);
        Value c;
        executeCallableExtern(thread->variable(0).object, args, sizeof(args) / sizeof(Value), thread);
        return c.raw < 0;
    });
    thread->returnFromFunction();
}

void listFromListBridge(Thread *thread) {
    auto listO = thread->retain(newObject(CL_LIST));

    auto *list = listO->val<List>();
    auto *originalList = thread->thisObject()->val<List>();

    list->count = originalList->count;
    list->capacity = originalList->capacity;

    Object *items = newArray(sizeof(Box) * originalList->capacity);
    list = listO->val<List>();
    originalList = thread->thisObject()->val<List>();
    list->items = items;

    std::memcpy(list->elements(), originalList->elements(), originalList->count * sizeof(Box));
    thread->release(1);
    thread->returnFromFunction(listO.unretainedPointer());
}

void listRemoveAllBridge(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();
    std::memset(list->elements(), 0, list->count * sizeof(Box));
    list->count = 0;
    thread->returnFromFunction();
}

void listSetBridge(Thread *thread) {
    EmojicodeInteger index = thread->variable(0).raw;
    listEnsureCapacity(thread, index + 1);
    auto *list = thread->thisObject()->val<List>();

    if (list->count <= index) {
        list->count = index + 1;
    }

    list->elements()[index].copy(thread->variableDestination(1));
    thread->returnFromFunction();
}

void listShuffleInPlaceBridge(Thread *thread) {
    auto *list = thread->thisObject()->val<List>();

    auto rng = std::mt19937_64(std::random_device()());
    std::shuffle(list->elements(), list->elements() + list->count, rng);

    thread->returnFromFunction();
}

void listEnsureCapacityBridge(Thread *thread) {
    listEnsureCapacity(thread, thread->variable(0).raw);
    thread->returnFromFunction();
}

void initListEmptyBridge(Thread *thread) {
    // The Real-Time Engine guarantees pre-nulled objects.
    thread->returnFromFunction(thread->thisContext());
}

void initListWithCapacity(Thread *thread) {
    EmojicodeInteger capacity = thread->variable(0).raw;
    Object *n = newArray(sizeCalculationWithOverflowProtection(capacity, sizeof(Box)));
    auto *list = thread->thisObject()->val<List>();
    list->capacity = capacity;
    list->items = n;
    thread->returnFromFunction(thread->thisContext());
}

}  // namespace Emojicode
