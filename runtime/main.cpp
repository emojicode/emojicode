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

runtime::internal::ControlBlock ejcIgnoreBlock;

int runtime::internal::argc;
char **runtime::internal::argv;

runtime::internal::ControlBlock* runtime::internal::newControlBlock() {
    return new runtime::internal::ControlBlock;
}

extern "C" runtime::Integer fn_1f3c1();

extern "C" int8_t* ejcAlloc(runtime::Integer size) {
    auto ptr = malloc(size);
    *static_cast<runtime::internal::ControlBlock**>(ptr) = new runtime::internal::ControlBlock;
    return static_cast<int8_t*>(ptr);
}

extern "C" void ejcRetain(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) {
        auto ptr = reinterpret_cast<int64_t *>(reinterpret_cast<uint8_t *>(object) - 8);
        (*ptr)++;
        return;
    }
    if (controlBlock == &ejcIgnoreBlock) return;
    controlBlock->strongCount++;
}

bool releaseLocal(void *object) {
    auto &ptr = *reinterpret_cast<int64_t *>(reinterpret_cast<uint8_t *>(object) - 8);
    ptr--;
    return ptr == 0;
}

extern "C" void ejcRelease(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) {
        if (releaseLocal(object)) {
            object->classInfo()->dispatch<void>(0, object);
        }
        return;
    }
    if (controlBlock == &ejcIgnoreBlock) return;

    controlBlock->strongCount--;
    if (controlBlock->strongCount != 0) return;

    object->classInfo()->dispatch<void>(0, object);
    delete controlBlock;
    free(object);
}

extern "C" void ejcReleaseCapture(runtime::internal::Capture *capture) {
    runtime::internal::ControlBlock *controlBlock = capture->controlBlock;
    if (controlBlock == nullptr) {
        if (releaseLocal(capture)) {
            capture->deinit(capture);
        }
        return;
    }
    if (controlBlock == &ejcIgnoreBlock) return;

    controlBlock->strongCount--;
    if (controlBlock->strongCount != 0) return;

    capture->deinit(capture);
    delete controlBlock;
    free(capture);
}

extern "C" void ejcReleaseMemory(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) {
        releaseLocal(object);
        return;
    }
    if (controlBlock == &ejcIgnoreBlock) return;

    controlBlock->strongCount--;
    if (controlBlock->strongCount != 0) return;

    delete controlBlock;
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

extern "C" bool ejcIsOnlyReference(runtime::Object<void> *object) {
    runtime::internal::ControlBlock *controlBlock = object->controlBlock();
    if (controlBlock == nullptr) {
        return *reinterpret_cast<int64_t *>(reinterpret_cast<uint8_t *>(object) - 8) == 1;
    }
    if (controlBlock == &ejcIgnoreBlock) return false;  // Impossible to say as object is not reference counted
    return controlBlock->strongCount == 1;
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
