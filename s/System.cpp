//
// Created by Theo Weidmann on 17.03.18.
//

#include "../runtime/Runtime.h"
#include "../runtime/Internal.hpp"
#include "String.h"
#include <cstdlib>
#include <ctime>

extern "C" [[noreturn]] void sSystemExit(runtime::ClassInfo*, runtime::Integer code) {
    std::exit(code);
}

extern "C" runtime::Integer sSystemUnixTimestamp(runtime::ClassInfo*) {
    return std::time(0);
}

extern "C" runtime::SimpleOptional<s::String*> sSystemGetEnv(runtime::ClassInfo*, s::String *name) {
    auto var = std::getenv(name->cString());
    if (var != nullptr) {
        return s::String::init(var);
    }
    return runtime::NoValue;
}

extern "C" [[noreturn]] void sPanic(runtime::ClassInfo*, s::String *message) {
    ejcPanic(message->cString());
}

extern "C" runtime::SimpleOptional<s::String*> sSystemArg(runtime::ClassInfo*, runtime::Integer i) {
    if (i >= runtime::internal::argc) {
        return runtime::NoValue;
    }
    return s::String::init(runtime::internal::argv[i]);
}

extern "C" void sSystemSystem(runtime::ClassInfo*, s::String *string) {
    std::system(string->cString());
}
