//
//  main.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include "Runtime.h"

extern "C" runtime::Integer fn_1f3c1();

extern "C" int ejcRunTimeClassValueTypeMeta = 0;

extern "C" int8_t* ejcAlloc(runtime::Integer size) {
    return static_cast<int8_t*>(malloc(size));
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

extern "C" void ejcErrNoValue(int64_t line, runtime::Integer character) {
    printf("💣 Fatal Error: Unwrapped an optional that contained no value. (line %" PRId64 ", character %"
           PRId64 ")\n", line, character);
    abort();
}

int main(int argc, char **argv) {
    auto code = fn_1f3c1();
    return static_cast<int>(code);
}
