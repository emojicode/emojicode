//
//  Object.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "Emojicode.h"
#include <string.h>

static size_t memoryUse = 0;
static bool zeroingNeeded = true;

static size_t gcThreshold = heapSize / 2;

static void* emojicodeMalloc(size_t size){
    if (memoryUse + size > gcThreshold) {
        if (size > gcThreshold) {
            error("Allocation of %ld bytes is too big. Try to enlarge the heap. (Heap size: %ld)", size, heapSize);
        }
        gc(mainThread);
    }
    Byte *block = currentHeap + memoryUse;
    memoryUse += size;
    return block;
}

static void* emojicodeRealloc(void *ptr, size_t oldSize, size_t newSize){
    //Nothing has been allocated since the allocation of ptr
    if (ptr == currentHeap + memoryUse - oldSize) {
        memoryUse += newSize - oldSize;
        return ptr;
    }
    
    void *block = emojicodeMalloc(newSize);
    memcpy(block, ptr, oldSize);
    return block;
}

static Object* newObjectWithSizeInternal(Class *class, size_t size){
    size_t fullSize = sizeof(Object) + size;
    Object *object = emojicodeMalloc(fullSize);
    object->size = fullSize;
    object->class = class;
    object->value = ((Byte *)object) + sizeof(Object) + class->instanceVariableCount * sizeof(Something);
    
    return object;
}

Something objectGetVariable(Object *o, uint8_t index){
    return *(Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index);
}

void objectSetVariable(Object *o, uint8_t index, Something value){
    Something *v = (Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index);
    *v = value;
}

void objectDecrementVariable(Object *o, uint8_t index){
    ((Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index))->raw--;
}

void objectIncrementVariable(Object *o, uint8_t index){
    ((Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index))->raw++;
}

Object* newObject(Class *class){
    return newObjectWithSizeInternal(class, class->size);
}

Object* newArray(size_t size){
    return newObjectWithSizeInternal(CL_ENUMERATOR, size);
}

Object* enlargeArray(Object *array, size_t size){
    size_t fullSize = sizeof(Object) + size;
    Object *object = emojicodeRealloc(array, array->size, fullSize);
    object->size = fullSize;
    object->value = ((Byte *)object) + sizeof(Object);
    return object;
}

void allocateHeap(){
    currentHeap = calloc(heapSize, 1);
    if (!currentHeap) {
        error("Cannot allocate heap!");
    }
    otherHeap = currentHeap + (heapSize / 2);
}

void mark(Object **oPointer){
    Object *o = *oPointer;
    if (o->newLocation) {
        *oPointer = o->newLocation;
        return;
    }
    
    o->newLocation = emojicodeMalloc(o->size);
    
    memcpy(o->newLocation, o, o->size);
    *oPointer = o->newLocation;
    
    o->newLocation->value = ((Byte *)o->newLocation) + sizeof(Object) + o->class->instanceVariableCount * sizeof(Something);
    
    //This class can lead the GC to other objects.
    if (o->class->mark) {
        o->class->mark(o->newLocation);
    }
}

void gc(Thread *thread){
    if (zeroingNeeded) {
        memset(otherHeap, 0, heapSize / 2);
    }
    else {
        zeroingNeeded = true;
    }
    
    //Set new location of all objects to NULL
    Byte *currentObjectPointer = currentHeap;
    while (currentObjectPointer < currentHeap + memoryUse - 1) {
        Object *currentObject = (Object *)currentObjectPointer;
        currentObject->newLocation = NULL;
        currentObjectPointer += currentObject->size;
    }
    
    void *tempHeap = currentHeap;
    currentHeap = otherHeap;
    otherHeap = tempHeap;
    size_t oldMemoryUse = memoryUse;
    memoryUse = 0;
    
    //Mark from rootset
    stackMark(thread);
    
    for (uint_fast16_t i = 0; i < stringPoolCount; i++) {
        mark(stringPool + i);
    }
    
    //Call the deinitializers
    currentObjectPointer = otherHeap;
    while (currentObjectPointer < otherHeap + oldMemoryUse) {
        Object *currentObject = (Object *)currentObjectPointer;
        if(!currentObject->newLocation && currentObject->class->deconstruct){
            currentObject->class->deconstruct(currentObject->value);
        }
        currentObjectPointer += currentObject->size;
    }
   
    if (oldMemoryUse == memoryUse) {
        error("Terminating program due to too high memory pressure.");
    }
}

bool instanceof(Object *object, Class *class){
    return inheritsFrom(object->class, class);
}

bool isPossibleObjectPointer(void *s){
    return (Byte *)s < currentHeap + heapSize/2 && s >= (void *)currentHeap;
}