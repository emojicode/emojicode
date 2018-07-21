//
//  main.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Runtime.h"
#include "Internal.hpp"
#include "backward.hpp"
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <iostream>

int runtime::internal::argc;
char **runtime::internal::argv;

extern "C" runtime::Integer fn_1f3c1();

extern "C" int8_t* ejcAlloc(runtime::Integer size) {
    return static_cast<int8_t*>(malloc(size));
}

extern "C" bool ejcInheritsFrom(runtime::ClassInfo *classInfo, runtime::ClassInfo *from) {
    for (auto classInfoNew = classInfo; classInfoNew != nullptr; classInfoNew = classInfoNew->superclass) {
        if (classInfoNew == from) {
            return true;
        }
    }
    return false;
}

struct BoxInfo {
    void *protocolId;
    void *protocolConformance;
};

extern "C" void* ejcFindProtocolConformance(BoxInfo *info, void *protocolId) {
    for (auto infoNew = info; infoNew->protocolId != nullptr; infoNew++) {
        if (infoNew->protocolId == protocolId) {
            return infoNew->protocolConformance;
        }
    }
    return nullptr;
}

extern "C" void ejcMemoryRealloc(int8_t **pointerPtr, runtime::Integer newSize) {
    *pointerPtr = static_cast<int8_t*>(realloc(*pointerPtr, newSize));
}

extern "C" void ejcMemoryMove(int8_t **self, runtime::Integer destOffset, int8_t *src, runtime::Integer srcOffset,
                              runtime::Integer bytes) {
    memmove(*self + destOffset, src + srcOffset, bytes);
}

extern "C" void ejcMemorySet(int8_t **self, runtime::Integer value, runtime::Integer offset, runtime::Integer bytes) {
    memset(*self + offset, value, bytes);
}

extern "C" runtime::Integer ejcMemoryCompare(int8_t **self, int8_t *other, runtime::Integer bytes) {
    return std::memcmp(*self, other, bytes);
}

extern "C" [[noreturn]] void ejcPanic(const char *message) {
    std::cout << "🤯 Program panicked: " << message << std::endl;

    backward::StackTrace st;
    st.load_here(32);
    backward::Printer p;
    p.print(st);

    abort();
}

int main(int largc, char **largv) {
    runtime::internal::argc = largc;
    runtime::internal::argv = largv;

    auto code = fn_1f3c1();
    return static_cast<int>(code);
}
