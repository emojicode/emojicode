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
#include <cmath>

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
    string->characters = runtime::allocate<char>(d);

    auto *characters = string->characters.get() + d;
    do {
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    } while ((a /= base) > 0);

    if (negative) {
        characters[-1] = '-';
    }
    return string;
}

extern "C" s::String* sRealToString(runtime::Real *real, runtime::Integer precision) {
    double integral;
    double fractional = std::modf(*real, &integral);

    if (precision <= 0) {
        runtime::Integer i = static_cast<runtime::Integer>(integral);
        return sIntToString(&i, 10);
    }

    auto a = std::abs(static_cast<long long>(integral));
    bool negative = integral < 0;

    unsigned int d = (negative ? 3 : 2) + precision;
    auto ac = a;
    while ((ac /= 10) != 0) {
        d++;
    }

    auto string = String::init();
    string->count = d;
    string->characters = runtime::allocate<char>(d);

    auto *characters = string->characters.get() + d;

    auto f = static_cast<long long>(std::abs(std::pow(10, precision) * fractional));
    for (decltype(precision) i = 0; i < precision; i++) {
        *--characters = "0123456789"[f % 10];
        f /= 10;
    }
    *--characters = '.';
    do {
        *--characters = "0123456789"[a % 10];
    } while ((a /= 10) > 0);
    if (negative) {
        characters[-1] = '-';
    }
    return string;
}

