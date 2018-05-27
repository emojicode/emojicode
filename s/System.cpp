//
// Created by Theo Weidmann on 17.03.18.
//

#include "../runtime/Runtime.h"
#include "../runtime/Internal.hpp"
#include "String.hpp"
#include <cstdlib>
#include <ctime>

extern "C" [[noreturn]] void sSystemExit(runtime::ClassType, runtime::Integer code) {
    std::exit(code);
}

extern "C" runtime::Integer sSystemUnixTimestamp(runtime::ClassType) {
    return std::time(0);
}

extern "C" runtime::SimpleOptional<s::String*> sSystemGetEnv(runtime::ClassType, s::String *name) {
    auto var = std::getenv(name->cString());
    if (var != nullptr) {
        return s::String::allocateAndInitType(var);
    }
    return runtime::NoValue;
}

extern "C" [[noreturn]] void sPanic(runtime::ClassType, s::String *message) {
    ejcPanic(message->cString());
}
