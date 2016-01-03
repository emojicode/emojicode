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

#define items(list) ((Something *)list->items->value)

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
    items(list)[list->count] = o;
    list->count++;
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
    for (size_t i = index + 1; i < list->count; i++) {
        items(list)[i - 1] = items(list)[i];
    }
    list->count--;
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
    //Addition has currently no use, but it may be used in the future to optimize the new size
    if (list->capacity == 0) {
        Object *object = newArray(sizeof(Something) * initialSize);
        list = stackGetThis(thread)->value;
        list->items = object;
        list->capacity = initialSize;
    }
    else {
        size_t newSize = list->capacity + (list->capacity >> 1);
        Object *object = enlargeArray(list->items, newSize * sizeof(Something));
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

static void constructEmptyListBridge(Thread *thread){
    //Nothing to do
    //The Real-Time Engine guarantees pre-nulled objects.
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
    
    for (size_t i = list->count - 1; i >= index; i--) {
        items(list)[i + 1] = items(list)[i];
    }
    items(list)[index] = stackGetVariable(1, thread);
    list->count++;
    
    return NOTHINGNESS;
}

static Something listFromListBridge(Thread *thread){
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
    }
    return NULL;
}

InitializerHandler listInitializerForName(EmojicodeChar method){
    return constructEmptyListBridge;
}
