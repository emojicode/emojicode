//
//  String.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../runtime/Runtime.h"
#include "String.hpp"
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include "utf8.h"

using s::String;

const char* String::cString() {
    size_t ds = u8_codingsize(characters, count);
    auto *utf8str = runtime::allocate<char>(ds + 1);
    // Convert
    size_t written = u8_toutf8(utf8str, ds, characters, count);
    utf8str[written] = 0;
    return utf8str;
}


int String::compare(String *other) {
    if (count != other->count) {
        return count > other->count ? 1 : -1;
    }

    return std::memcmp(characters, other->characters, count * sizeof(Character));
}

extern "C" void sStringPrint(String *string) {
    puts(string->cString());
}

extern "C" void sCodePointToString(String *string) {
    CString codepoint(string->cString());
    std::wstring codepoint2(codepoint);
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cnvt;
    std::string stringout = cnvt.to_bytes(codepoint2);
    return stringout.c_str();
}

extern "C" runtime::Integer sStringCompare(String *string, String *b) {
    return string->compare(b);
}

extern "C" char sStringEqual(String *string, String *b) {
    return string->compare(b) == 0;
}

extern "C" char sStringBeginsWith(String *string, String *beginning) {
    if (string->count < beginning->count) {
        return false;
    }
    return std::memcmp(string->characters, beginning->characters,
                       beginning->count * sizeof(String::Character)) == 0;
}

extern "C" char sStringEndsWith(String *string, String *ending) {
    if (string->count < ending->count) {
        return false;
    }
    return std::memcmp(string->characters + (string->count - ending->count), ending->characters,
                       ending->count * sizeof(String::Character)) == 0;
}

extern "C" runtime::Integer sStringUtf8ByteCount(String *string) {
    return u8_codingsize(string->characters, string->count);
}
