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
#include <cctype>
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

String* String::fromCString(const char *cstring) {
    auto len = u8_strlen(cstring);

    auto *string = String::allocateAndInitType();
    string->count = len;

    if (len == 0) {
        return string;
    }

    string->characters = runtime::allocate<String::Character>(string->count);
    u8_toucs(string->characters, string->count, cstring, strlen(cstring));

    return string;
}

extern "C" void sStringPrint(String *string) {
    puts(string->cString());
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

extern "C" String* sStringToLowercase(String *string) {
    auto newString = String::allocateAndInitType();
    newString->count = string->count;
    newString->characters = runtime::allocate<String::Character>(string->count);

    for (runtime::Integer i = 0; i < newString->count; i++) {
        auto codePoint = string->characters[i];
        newString->characters[i] = codePoint <= 'z' ? std::tolower(static_cast<unsigned char>(codePoint)) : codePoint;
    }

    return newString;
}

extern "C" String* sStringToUppercase(String *string) {
    auto newString = String::allocateAndInitType();
    newString->count = string->count;
    newString->characters = runtime::allocate<String::Character>(string->count);

    for (runtime::Integer i = 0; i < newString->count; i++) {
        auto codePoint = string->characters[i];
        newString->characters[i] = codePoint <= 'z' ? std::toupper(static_cast<unsigned char>(codePoint)) : codePoint;
    }

    return newString;
}

extern "C" String* sStringAppendSymbol(String *string, runtime::Symbol symbol) {
    auto characters = runtime::allocate<String::Character>(string->count + 1);

    std::memcpy(characters, string->characters, string->count * sizeof(String::Character));
    characters[string->count] = symbol;

    auto newString = String::allocateAndInitType();
    newString->count = string->count + 1;
    newString->characters = characters;
    return newString;
}