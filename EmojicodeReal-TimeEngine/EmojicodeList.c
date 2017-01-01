//
//  List.c
//  Emojicode
//
//  Created by Theo Weidmann on 18.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeList.h"
#include "EmojicodeString.h"

#include <string.h>

#define items(list) ((Something *)(list)->items->value)

void expandListSize(Thread *thread){
#define initialSize 7
    List *list = stackGetThisObject(thread)->value;
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Something) * initialSize);
        list = stackGetThisObject(thread)->value;
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = resizeArray(list->items, sizeCalculationWithOverflowProtection(newSize, sizeof(Something)));
        list = stackGetThisObject(thread)->value;
        list->items = object;
        list->capacity = newSize;
    }
#undef initialSize
}

void listEnsureCapacity(Thread *thread, size_t size) {
    List *list = stackGetThisObject(thread)->value;
    if (list->capacity < size) {
        Object *object;
        if (list->capacity == 0) {
            object = newArray(sizeCalculationWithOverflowProtection(size, sizeof(Something)));
        }
        else {
            object = resizeArray(list->items, sizeCalculationWithOverflowProtection(size, sizeof(Something)));
        }
        list = stackGetThisObject(thread)->value;
        list->items = object;
        list->capacity = size;
    }
}

void listMark(Object *self){
    List *list = self->value;
    if (list->items) {
        mark(&list->items); 
    }
    for (size_t i = 0; i < list->count; i++) {
        if(isRealObject(items(list)[i]))
           mark(&items(list)[i].object);
    }
}

void listAppend(Object *lo, Something o, Thread *thread){
    stackPush(somethingObject(lo), 0, 0, thread);
    List *list = lo->value;
    if (list->capacity - list->count == 0) {
        expandListSize(thread);
    }
    list = stackGetThisObject(thread)->value;
    items(list)[list->count++] = o;
    stackPop(thread);
}

Something listPop(List *list){
    if(list->count == 0){
        return NOTHINGNESS;
    }
    size_t index = --list->count;
    Something v = items(list)[index];
    items(list)[index] = NOTHINGNESS;
    return v;
}

bool listRemoveByIndex(List *list, EmojicodeInteger index){
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index){
        return false;
    }
    memmove(items(list) + index, items(list) + index + 1, sizeof(Something) * (list->count - index - 1));
    items(list)[--list->count] = NOTHINGNESS;
    return true;
}

Something listGet(List *list, EmojicodeInteger i){
    if (i < 0) {
        i += list->count;
    }
    if (i < 0 || list->count <= i){
        return NOTHINGNESS;
    }
    return items(list)[i];
}

Something listSet(EmojicodeInteger index, Something value, Thread *thread) {
    List *list = stackGetThisObject(thread)->value;
    
    listEnsureCapacity(thread, index + 1);
    list = stackGetThisObject(thread)->value;
    
    if (list->count <= index)
        list->count = index + 1;
    
    items(list)[index] = value;
    return NOTHINGNESS;
}

void listShuffleInPlace(List *list) {
    EmojicodeInteger i, j, n = (EmojicodeInteger)list->count;
    Something tmp;
    
    for (i = n - 1; i > 0; i--) {
        j = secureRandomNumber(0, i);
        tmp = items(list)[j];
        items(list)[j] = items(list)[i];
        items(list)[i] = tmp;
    }
}

/* MARK: Emoji bridges */

void listCountBridge(Thread *thread, Something *destination) {
    *destination = somethingInteger((EmojicodeInteger)((List *)stackGetThisObject(thread)->value)->count);
}

void listAppendBridge(Thread *thread, Something *destination) {
    listAppend(stackGetThisObject(thread), stackGetVariable(0, thread), thread);
}

void listGetBridge(Thread *thread, Something *destination) {
    *destination = listGet(stackGetThisObject(thread)->value, unwrapInteger(stackGetVariable(0, thread)));
}

void listRemoveBridge(Thread *thread, Something *destination) {
    *destination = listRemoveByIndex(stackGetThisObject(thread)->value, unwrapInteger(stackGetVariable(0, thread))) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

void listPopBridge(Thread *thread, Something *destination) {
    *destination = listPop(stackGetThisObject(thread)->value);
}

void listInsertBridge(Thread *thread, Something *destination) {
    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    List *list = stackGetThisObject(thread)->value;
    
    if (index < 0) {
        index += list->count;
    }
    if (index < 0 || list->count <= index){
        *destination = NOTHINGNESS;
    }
    
    if (list->capacity - list->count == 0) {
        expandListSize(thread);
    }
    
    list = stackGetThisObject(thread)->value;
    
    memmove(items(list) + index + 1, items(list) + index, sizeof(Something) * (list->count++ - index));
    items(list)[index] = stackGetVariable(1, thread);
}

void listQSort(Thread *thread, size_t off, size_t n) {
    if (n < 2)
        return;
    
    Something *items = items((List *)stackGetThisObject(thread)->value) + off;
    Something pivot = items[n / 2];
    size_t i, j;
    
    for (i = 0, j = n - 1; ; i++, j--) {
        while (true) {
            Something args[2] = {items[i], pivot};
            Something c;
            executeCallableExtern(stackGetVariable(0, thread).object, args, thread, &c);
            items = items((List *)stackGetThisObject(thread)->value) + off;
            if (c.raw >= 0) break;
            i++;
        }
        
        while (true) {
            Something args[2] = {pivot, items[j]};
            Something c;
            executeCallableExtern(stackGetVariable(0, thread).object, args, thread, &c);
            items = items((List *)stackGetThisObject(thread)->value) + off;
            if (c.raw >= 0) break;
            j--;
        }
        
        if (i >= j)
            break;
        
        Something temp = items[i];
        items[i] = items[j];
        items[j] = temp;
    }
    
    listQSort(thread, off, i);
    listQSort(thread, off + i, n - i);
}

void listSort(Thread *thread, Something *destination) {
    List *list = stackGetThisObject(thread)->value;
    listQSort(thread, 0, list->count);
}

void listFromListBridge(Thread *thread, Something *destination) {
    Object *listO = newObject(CL_LIST);
    stackPush(stackGetThisContext(thread), 1, 0, thread);
    *stackVariableDestination(0, thread) = somethingObject(listO);

    List *list = listO->value;
    List *cpdList = stackGetThisObject(thread)->value;
    
    list->count = cpdList->count;
    list->capacity = cpdList->capacity;
    
    Object *items = newArray(sizeof(Something) * cpdList->capacity);
    listO = stackGetVariable(0, thread).object;
    list = listO->value;
    cpdList = stackGetThisObject(thread)->value;
    list->items = items;
    
    memcpy(items(list), items(cpdList), cpdList->count * sizeof(Something));
    stackPop(thread);
    *destination = somethingObject(listO);
}

void listRemoveAllBridge(Thread *thread, Something *destination) {
    List *list = stackGetThisObject(thread)->value;
    memset(items(list), 0, list->count);
    list->count = 0;
}

void listSetBridge(Thread *thread, Something *destination) {
    listSet(stackGetVariable(0, thread).raw, stackGetVariable(1, thread), thread);
}

void listShuffleInPlaceBridge(Thread *thread, Something *destination) {
    listShuffleInPlace(stackGetThisObject(thread)->value);
}

void listEnsureCapacityBridge(Thread *thread, Something *destination) {
    listEnsureCapacity(thread, stackGetVariable(0, thread).raw);
}

void initListEmptyBridge(Thread *thread, Something *destination) {
    //Nothing to do
    //The Real-Time Engine guarantees pre-nulled objects.
}

void initListWithCapacity(Thread *thread, Something *destination) {
    EmojicodeInteger capacity = stackGetVariable(0, thread).raw;
    Object *n = newArray(sizeCalculationWithOverflowProtection(capacity, sizeof(Something)));
    List *list = stackGetThisObject(thread)->value;
    list->capacity = capacity;
    list->items = n;
}
