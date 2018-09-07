//
//  main.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Runtime.h"
#include "Internal.hpp"
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <iostream>

int runtime::internal::argc;
char **runtime::internal::argv;

extern "C" runtime::Integer fn_1f3c1();

extern "C" int8_t* ejcAlloc(runtime::Integer size) {
    auto ptr = malloc(size);
    *static_cast<runtime::internal::ControlBlock**>(ptr) = new runtime::internal::ControlBlock;
//    std::cout << "Allocated " << ptr << std::endl;
    return static_cast<int8_t*>(ptr);
}

extern "C" void ejcRetain(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
//    std::cout << "Retaining " << object << std::endl;
    if (controlBlock == nullptr) return;
    controlBlock->strongCount++;
}

extern "C" void ejcRelease(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) return;

//    std::cout << "Releasing " << object << std::endl;

    controlBlock->strongCount--;
//        std::cout << controlBlock->strongCount << std::endl;
    if (controlBlock->strongCount != 0) return;

//    std::cout << "Deleting " << object << std::endl;
    object->classInfo()->dispatch<void>(0, object);
    free(object);
}

extern "C" void ejcReleaseMemory(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) return;
//    std::cout << "Releasing " << object << std::endl;

    controlBlock->strongCount--;
//    std::cout << controlBlock->strongCount << std::endl;
    if (controlBlock->strongCount != 0) return;

//    std::cout << "Deleting " << object << std::endl;
    free(object);
}

extern "C" bool ejcInheritsFrom(runtime::ClassInfo *classInfo, runtime::ClassInfo *from) {
    for (auto classInfoNew = classInfo; classInfoNew != nullptr; classInfoNew = classInfoNew->superclass) {
        if (classInfoNew == from) {
            return true;
        }
    }
    return false;
}

struct ProtocolConformanceEntry {
    void *protocolId;
    void *protocolConformance;
};

extern "C" void* ejcFindProtocolConformance(ProtocolConformanceEntry *info, void *protocolId) {
    for (auto infoNew = info; infoNew->protocolId != nullptr; infoNew++) {
        if (infoNew->protocolId == protocolId) {
            return infoNew->protocolConformance;
        }
    }
    return nullptr;
}

extern "C" void ejcMemoryRealloc(int8_t **pointerPtr, runtime::Integer newSize) {
    *pointerPtr = static_cast<int8_t*>(realloc(*pointerPtr, newSize + sizeof(runtime::internal::ControlBlock*)));
}

extern "C" runtime::Integer ejcMemoryCompare(int8_t **self, int8_t *other, runtime::Integer bytes) {
    return std::memcmp(*self + sizeof(runtime::internal::ControlBlock*),
                       other + sizeof(runtime::internal::ControlBlock*), bytes);
}

extern "C" [[noreturn]] void ejcPanic(const char *message) {
    std::cout << "🤯 Program panicked: " << message << std::endl;
    abort();
}

int main(int largc, char **largv) {
    runtime::internal::argc = largc;
    runtime::internal::argv = largv;

    auto code = fn_1f3c1();
    return static_cast<int>(code);
}
