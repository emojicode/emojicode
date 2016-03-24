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

void expandListSize(Thread *thread);

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
    stackPush(lo, 0, 0, thread);
    List *list = lo->value;
    if (list->capacity - list->count == 0) {
        expandListSize(thread);
    }
    list = stackGetThis(thread)->value;
    items(list)[list->count++] = o;
    stackPop(thread);
}

Something listPop(List *list){
    if(list->count == 0){
        return NOTHINGNESS;
    }
    Something o = items(list)[list->count - 1];
    list->count--;
    return o;
}

bool listRemoveByIndex(List *list, size_t index){
    if (list->count <= index){
        return false;
    }
    memmove(items(list) + index, items(list) + index + 1, sizeof(Something) * (--list->count - index));
    return true;
}

Something listGet(List *list, size_t i){
    if (list->count <= i){
        return NOTHINGNESS;
    }
    return items(list)[i];
}

void expandListSize(Thread *thread){
#define initialSize 7
    List *list = stackGetThis(thread)->value;
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Something) * initialSize);
        list = stackGetThis(thread)->value;
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = resizeArray(list->items, newSize * sizeof(Something));
        list = stackGetThis(thread)->value;
        list->items = object;
        list->capacity = newSize;
    }
#undef initialSize
}

bool listRemove(List *list, Something x){
    if(x.type != T_OBJECT){
        for(size_t i = 0; i < list->count; i++){
            if(items(list)[i].type == x.type && items(list)[i].raw == x.raw){
                listRemoveByIndex(list, i);
                return true;
            }
        }
    }
    else {
        for(size_t i = 0; i < list->count; i++){
            if(items(list)[i].type == T_OBJECT && items(list)[i].object == x.object){
                listRemoveByIndex(list, i);
                return true;
            }
        }
    }
    
    return false;
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

static Something listCountBridge(Thread *thread){
    return somethingInteger((EmojicodeInteger)((List *)stackGetThis(thread)->value)->count);
}

static Something listAppendBridge(Thread *thread){
    listAppend(stackGetThis(thread), stackGetVariable(0, thread), thread);
    return NOTHINGNESS;
}

static Something listGetBridge(Thread *thread){
    return listGet(stackGetThis(thread)->value, unwrapInteger(stackGetVariable(0, thread)));
}

static Something listRemoveBridge(Thread *thread){
    return listRemoveByIndex(stackGetThis(thread)->value, unwrapInteger(stackGetVariable(0, thread))) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something listPopBridge(Thread *thread){
    return listPop(stackGetThis(thread)->value);
}

static Something listInsertBridge(Thread *thread){
    List *list = stackGetThis(thread)->value;
    if (list->capacity - list->count == 0) {
        expandListSize(thread);
    }
    
    list = stackGetThis(thread)->value;
    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    
    memmove(items(list) + index + 1, items(list) + index, sizeof(Something) * (list->count++ - index));
    items(list)[index] = stackGetVariable(1, thread);
    
    return NOTHINGNESS;
}

static void listQSort(Thread *thread, size_t off, size_t n) {
    if (n < 2)
        return;
    
    Something *items = items((List *)stackGetThis(thread)->value) + off;
    Something pivot = items[n / 2];
    size_t i, j;
    
    for (i = 0, j = n - 1; ; i++, j--) {
        while (true) {
            Something args[2] = {items[i], pivot};
            EmojicodeInteger c = executeCallableExtern(stackGetVariable(0, thread).object, args, thread).raw;
            items = items((List *)stackGetThis(thread)->value) + off;
            if (c >= 0) break;
            i++;
        }
        
        while (true) {
            Something args[2] = {pivot, items[j]};
            EmojicodeInteger c = executeCallableExtern(stackGetVariable(0, thread).object, args, thread).raw;
            items = items((List *)stackGetThis(thread)->value) + off;
            if (c >= 0) break;
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

static Something listSort(Thread *thread){
    List *list = stackGetThis(thread)->value;
    listQSort(thread, 0, list->count);
    return NOTHINGNESS;
}

static Something listFromListBridge(Thread *thread){ //TODO: GC_Safe
    Object *listO = newObject(CL_LIST);
    List *list = listO->value;
    List *cpdList = stackGetThis(thread)->value;
    
    list->count = cpdList->count;
    list->capacity = cpdList->capacity;
    list->items = newArray(sizeof(Something) * cpdList->capacity);
    
    memcpy(items(list), items(cpdList), cpdList->count * sizeof(Something));
    return somethingObject(listO);
}

static Something listShuffleInPlaceBridge(Thread *thread){
    listShuffleInPlace(stackGetThis(thread)->value);
    return NOTHINGNESS;
}

static void initListEmptyBridge(Thread *thread){
    //Nothing to do
    //The Real-Time Engine guarantees pre-nulled objects.
}

static void initListWithCapacity(Thread *thread){
    EmojicodeInteger capacity = stackGetVariable(0, thread).raw;
    Object *n = newArray(sizeof(Something) * capacity);
    List *list = stackGetThis(thread)->value;
    list->capacity = capacity;
    list->items = n;
}

MethodHandler listMethodForName(EmojicodeChar method){
    switch (method) {
        case 0x1F43B: //bear
            return listAppendBridge;
        case 0x1F43D: //pig nose
            return listGetBridge;
        case 0x1F428: //koala
            return listRemoveBridge;
        case 0x1F435: //monkey
            return listInsertBridge;
        case 0x1F414: //chicken
            return listCountBridge;
        case 0x1F43C: //panda
            return listPopBridge;
        case 0x1F439: //🐹
            return listShuffleInPlaceBridge;
        case 0x1F42E: //🐮
            return listFromListBridge;
        case 0x1F981: //🦁
            return listSort;
    }
    return NULL;
}

InitializerHandler listInitializerForName(EmojicodeChar name){
    switch (name) {
        case 0x1F427: //🐧
            return initListWithCapacity;
            break;
        default:
            return initListEmptyBridge;
            break;
    }
}
