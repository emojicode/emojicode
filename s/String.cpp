//
//  String.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../runtime/Runtime.h"
#include "String.hpp"
#include "Data.hpp"
#include <cinttypes>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cmath>
#include <algorithm>
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

String::String(const char *cstring) {
    count = u8_strlen(cstring);

    if (count > 0) {
        characters = runtime::allocate<String::Character>(count);
        u8_toucs(characters, count, cstring, strlen(cstring));
    }
}

extern "C" void sStringPrint(String *string) {
    std::cout << string->cString() << std::endl;
}

extern "C" void sStringPrintNoLn(String *print) {
    std::cout << print->cString();
}

extern "C" String* sStringReadLine(String *string) {
    std::string str;
    std::getline(std::cin, str);
    *string = str.c_str();
    return string;
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

extern "C" runtime::SimpleOptional<runtime::Integer> sStringFind(String *string, String* search) {
    auto end = string->characters + string->count;
    auto pos = std::search(string->characters, end, search->characters, search->characters + search->count);
    if (pos != end) {
        return pos - string->characters;
    }
    return runtime::NoValue;
}

extern "C" runtime::SimpleOptional<runtime::Integer> sStringFindFromIndex(String *string, String* search,
                                                                          runtime::Integer offset) {
    if (offset >= string->count) {
        return runtime::NoValue;
    }
    auto end = string->characters + string->count;
    auto pos = std::search(string->characters + offset, end, search->characters, search->characters + search->count);
    if (pos != end) {
        return pos - string->characters;
    }
    return runtime::NoValue;
}

extern "C" runtime::SimpleOptional<runtime::Integer> sStringFindSymbolFromIndex(String *string, runtime::Symbol search,
                                                                                runtime::Integer offset) {
    if (offset >= string->count) {
        return runtime::NoValue;
    }
    auto end = string->characters + string->count;
    auto pos = std::find(string->characters + offset, end, search);
    if (pos != end) {
        return pos - string->characters;
    }
    return runtime::NoValue;
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

extern "C" s::Data* sStringToData(String *string) {
    auto data = s::Data::allocateAndInitType();
    data->count = u8_codingsize(string->characters, string->count);
    data->data = runtime::allocate<runtime::Byte>(data->count);
    u8_toutf8(reinterpret_cast<char *>(data->data), data->count, string->characters, string->count);
    return data;
}

runtime::SimpleOptional<runtime::Integer> sStringToIntLength(const String::Character *characters,
                                                             runtime::Integer length, runtime::Integer base) {
    if (length == 0) {
        return runtime::NoValue;
    }
    runtime::Integer x = 0;
    for (decltype(length) i = 0; i < length; i++) {
        if (i == 0 && (characters[i] == '-' || characters[i] == '+')) {
            if (length < 2) {
                return runtime::NoValue;
            }
            continue;
        }

        auto b = base;
        if ('0' <= characters[i] && characters[i] <= '9') {
            b = characters[i] - '0';
        }
        else if ('A' <= characters[i] && characters[i] <= 'Z') {
            b = characters[i] - 'A' + 10;
        }
        else if ('a' <= characters[i] && characters[i] <= 'z') {
            b = characters[i] - 'a' + 10;
        }

        if (b >= base) {
            return runtime::NoValue;
        }

        x *= base;
        x += b;
    }

    if (characters[0] == '-') {
        x *= -1;
    }
    return x;
}

extern "C" runtime::SimpleOptional<runtime::Integer> sStringToInt(String *string, runtime::Integer base) {
    return sStringToIntLength(string->characters, string->count, base);
}

extern "C" runtime::SimpleOptional<runtime::Real> sStringToReal(String *string) {
    if (string->count == 0) {
        return runtime::NoValue;
    }

    runtime::Real d = 0.0;
    bool sign = true;
    bool foundSeparator = false;
    bool foundDigit = false;
    size_t decimalPlace = 0;
    decltype(string->count) i = 0;

    if (string->characters[0] == '-') {
        sign = false;
        i++;
    }
    else if (string->characters[0] == '+') {
        i++;
    }

    for (; i < string->count; i++) {
        if (string->characters[i] == '.') {
            if (foundSeparator) {
                return runtime::NoValue;
            }
            foundSeparator = true;
            continue;
        }
        if (string->characters[i] == 'e' || string->characters[i] == 'E') {
            auto exponent = sStringToIntLength(string->characters + i + 1, string->count - i - 1, 10);
            if (exponent == runtime::NoValue) {
                return runtime::NoValue;
            }
            d *= std::pow(10, *exponent);
            break;
        }
        if ('0' <= string->characters[i] && string->characters[i] <= '9') {
            d *= 10;
            d += string->characters[i] - '0';
            if (foundSeparator) {
                decimalPlace++;
            }
            foundDigit = true;
        }
        else {
            return runtime::NoValue;
        }
    }

    if (!foundDigit) {
        return runtime::NoValue;
    }

    d /= std::pow(10, decimalPlace);

    if (!sign) {
        d *= -1;
    }
    return d;
}

extern "C" runtime::Integer sStringHash(String *string) {
    auto len = sizeof(String::Character) * string->count;
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    unsigned int h = 29190 ^ len;

    const unsigned char * data = reinterpret_cast<const unsigned char *>(string->characters);

    while(len >= 4) {
        unsigned int k;

        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}
