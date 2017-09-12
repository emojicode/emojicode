//
//  String.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../Runtime/Runtime.h"
#include "String.hpp"
#include <cinttypes>
#include <cstdio>
#include "utf8.h"

using s::String;

const char* String::cString() {
    size_t ds = u8_codingsize(characters, count);
    auto *utf8str = reinterpret_cast<char*>(ejcAlloc(ds + 1));
    // Convert
    size_t written = u8_toutf8(utf8str, ds, characters, count);
    utf8str[written] = 0;
    return utf8str;
}

extern "C" void sStringPrint(String *string) {
    puts(string->cString());
}
