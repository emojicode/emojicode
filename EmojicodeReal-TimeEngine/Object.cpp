//
//  Object.c
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Object.hpp"
#include "Class.hpp"
#include "Engine.hpp"
#include "Thread.hpp"
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

namespace Emojicode {

size_t memoryUse = 0;
bool zeroingNeeded = false;
Byte *currentHeap;
Byte *otherHeap;

void gc(std::unique_lock<std::mutex> &allocationLock);

size_t gcThreshold = heapSize / 2;

int pausingThreadsCount = 0;
bool pauseThreads = false;  // Must only be set after a allocationMutex lock was obtained
std::mutex pausingThreadsCountMutex;
std::mutex allocationMutex;
std::condition_variable pauseThreadsCondition;
std::condition_variable pausingThreadsCountCondition;

Object* allocateObject(size_t size) {
    // We must obtain this mutex lock first to avoid a race condition on pauseThreads
    std::unique_lock<std::mutex> lock(allocationMutex);
    pauseForGC();
    if (memoryUse + size > gcThreshold) {
        if (size > gcThreshold) {
            error("Allocation of %zu bytes is too big. Try to enlarge the heap. (Heap size: %zu)", size, heapSize);
        }

        gc(lock);
    }
    Byte *block = currentHeap + memoryUse;
    memoryUse += size;
    return reinterpret_cast<Object *>(block);
}

Object* resizeObject(Object *ptr, size_t oldSize, size_t newSize) {
    // We must obtain this mutex lock first to avoid a race condition on pauseThreads
    std::unique_lock<std::mutex> lock(allocationMutex);
    pauseForGC();
    // Nothing has been allocated since the allocation of ptr
    if (ptr == reinterpret_cast<Object *>(currentHeap + memoryUse - oldSize) && memoryUse + newSize < gcThreshold) {
        memoryUse += newSize - oldSize;
        return ptr;
    }
    lock.unlock();
    Object *block = allocateObject(newSize);
    std::memcpy(block, ptr, oldSize);
    return block;
}

Object* newObject(Class *klass) {
    size_t fullSize = sizeof(Object) + klass->size;
    Object *object = allocateObject(fullSize);
    object->size = fullSize;
    object->klass = klass;
    return object;
}

size_t sizeCalculationWithOverflowProtection(size_t items, size_t itemSize) {
    size_t r = items * itemSize;
    if (r / items != itemSize) {
        error("Integer overflow while allocating memory. It’s not possible to allocate objects of this size due to "
              "hardware limitations.");
    }
    return r;
}

Object* newArray(size_t size) {
    size_t fullSize = sizeof(Object) + size;
    Object *object = allocateObject(fullSize);
    object->size = fullSize;
    object->klass = CL_ARRAY;
    return object;
}

Object* resizeArray(Object *array, size_t size) {
    size_t fullSize = sizeof(Object) + size;
    Object *object = resizeObject(array, array->size, fullSize);
    object->size = fullSize;
    return object;
}

void allocateHeap() {
    currentHeap = static_cast<Byte *>(calloc(heapSize, 1));
    if (!currentHeap) {
        error("Cannot allocate heap!");
    }
    otherHeap = currentHeap + (heapSize / 2);
}

inline bool inNewHeap(Object *o) {
    return currentHeap <= reinterpret_cast<Byte *>(o) && reinterpret_cast<Byte *>(o) < currentHeap + heapSize / 2;
}

void mark(Object **oPointer) {
    Object *oldObject = *oPointer;
    if (inNewHeap(oldObject->newLocation)) {
        *oPointer = oldObject->newLocation;
        return;
    }

    auto *newObject = reinterpret_cast<Object *>(currentHeap + memoryUse);
    memoryUse += oldObject->size;

    std::memcpy(newObject, oldObject, oldObject->size);

    oldObject->newLocation = newObject;
    *oPointer = newObject;
}

void gc(std::unique_lock<std::mutex> &allocationLock) {
    pauseThreads = true;
    allocationLock.unlock();

    auto pausingThreadsCountLock = std::unique_lock<std::mutex>(pausingThreadsCountMutex);
    pausingThreadsCount++;

    pausingThreadsCountCondition.wait(pausingThreadsCountLock, []{ return pausingThreadsCount == Thread::threads(); });

    std::swap(currentHeap, otherHeap);

    size_t oldMemoryUse = memoryUse;
    memoryUse = 0;

    for (Thread *thread = Thread::lastThread(); thread != nullptr; thread = thread->threadBefore()) {
        thread->markStack();
        thread->markRetainList();
    }

    for (uint_fast16_t i = 0; i < stringPoolCount; i++) {
        mark(stringPool + i);
    }

    for (Byte *byte = currentHeap; byte < currentHeap + memoryUse;) {
        auto object = reinterpret_cast<Object *>(byte);

        for (size_t i = 0; i < object->klass->instanceVariableRecordsCount; i++) {
            auto record = object->klass->instanceVariableRecords[i];
            markByObjectVariableRecord(record, object->variableDestination(0), i);
        }

        if (object->klass->mark != nullptr) {
            object->klass->mark(object);
        }
        byte += object->size;
    }

    if (oldMemoryUse == memoryUse) {
        error("Terminating program due to too high memory pressure.");
    }

    if (zeroingNeeded) {
        std::memset(currentHeap + memoryUse, 0, (heapSize / 2) - memoryUse);
    }
    else {
        zeroingNeeded = true;
    }

    pausingThreadsCount--;
    pausingThreadsCountLock.unlock();
    pauseThreads = false;
    pauseThreadsCondition.notify_all();
    allocationLock.lock();
}

void pauseForGC() {
    if (pauseThreads) {
        auto pausingThreadsCountLock = std::unique_lock<std::mutex>(pausingThreadsCountMutex);
        pausingThreadsCount++;
        pausingThreadsCountCondition.notify_one();
        pauseThreadsCondition.wait(pausingThreadsCountLock, []{ return !pauseThreads; });
        pausingThreadsCount--;
    }
}

void allowGC() {
    std::unique_lock<std::mutex> pausingThreadsCountLock(pausingThreadsCountMutex);
    pausingThreadsCount++;
    pausingThreadsCountCondition.notify_one();
}

void disallowGCAndPauseIfNeeded() {
    auto pausingThreadsCountLock = std::unique_lock<std::mutex>(pausingThreadsCountMutex);
    pauseThreadsCondition.wait(pausingThreadsCountLock, []{ return !pauseThreads; });
    pausingThreadsCount--;
    pausingThreadsCountCondition.notify_one();
}

}  // namespace Emojicode
