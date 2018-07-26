//
//  Integer.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../runtime/Runtime.h"
#include "String.h"
#include <cstdint>
#include <cstdlib>

using s::String;

extern "C" runtime::Integer sIntAbsolute(runtime::Integer *integer) {
    return std::abs(*integer);
}

extern "C" s::String* sIntToString(runtime::Integer *nptr, runtime::Integer base) {
    auto n = *nptr;
    auto a = std::abs(n);
    bool negative = n < 0;

    unsigned int d = negative ? 2 : 1;
    while ((n /= base) != 0) {
        d++;
    }

    auto string = String::init();
    string->count = d;
    string->characters = runtime::allocate<String::Character>(d);

    auto *characters = string->characters + d;
    do {
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    } while ((a /= base) > 0);

    if (negative) {
        characters[-1] = '-';
    }
    return string;
}

extern "C" runtime::Symbol sIntToSymbol(runtime::Integer *integer) {
    return static_cast<runtime::Symbol>(*integer);
}

extern "C" runtime::Byte sIntToByte(runtime::Integer *integer) {
    return static_cast<runtime::Byte>(*integer);
}
