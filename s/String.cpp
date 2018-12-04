//
//  String.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../runtime/Runtime.h"
#include "../runtime/Internal.hpp"
#include "Data.h"
#include "String.h"
#include "utf8proc.h"
#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iostream>

using s::String;

std::string String::stdString() {
    return std::string(characters.get(), count);
}

String::String(const char *cstring) {
    store(cstring);
}

void String::store(const char *cstring) {
    count = strlen(cstring);
    characters = runtime::allocate<char>(count);
    std::memcpy(characters.get(), cstring, count);
}

extern "C" void sStringPrint(String *string) {
    std::cout.write(string->characters.get(), string->count) << '\n';
}

extern "C" void sStringPrintNoLn(String *string) {
    std::cout.write(string->characters.get(), string->count);
}

extern "C" String* sStringReadLine(String *string) {
    std::string str;
    std::getline(std::cin, str);
    string->store(str.c_str());
    return string;
}

extern "C" char sStringBeginsWith(String *string, String *beginning) {
    if (string->count < beginning->count) {
        return false;
    }
    return std::memcmp(string->characters.get(), beginning->characters.get(), beginning->count) == 0;
}

extern "C" char sStringEndsWith(String *string, String *ending) {
    if (string->count < ending->count) {
        return false;
    }
    return std::memcmp(string->characters.get() + (string->count - ending->count), ending->characters.get(),
                       ending->count) == 0;
}

extern "C" runtime::Integer sStringUtf8ByteCount(String *string) {
    return string->count;
}

extern "C" String* sStringToLowercase(String *string) {
    auto newString = String::init();
    newString->count = string->count;
    newString->characters = runtime::allocate<char>(string->count);

    size_t doff = 0;
    for (size_t off = 0; off < string->count;) {
        utf8proc_int32_t codepoint;
        auto state = utf8proc_iterate(reinterpret_cast<utf8proc_uint8_t *>(string->characters.get()) + off,
                                      string->count, &codepoint);
        if (state < 0) break;
        doff += utf8proc_encode_char(utf8proc_tolower(codepoint),
                                     reinterpret_cast<utf8proc_uint8_t *>(newString->characters.get()) + doff);
        off += state;
    }
    return newString;
}

extern "C" String* sStringToUppercase(String *string) {
    auto newString = String::init();
    newString->count = string->count;
    newString->characters = runtime::allocate<char>(string->count);

    size_t doff = 0;
    for (size_t off = 0; off < string->count;) {
        utf8proc_int32_t codepoint;
        auto state = utf8proc_iterate(reinterpret_cast<utf8proc_uint8_t *>(string->characters.get()) + off,
                                      string->count, &codepoint);
        if (state < 0) break;
        doff += utf8proc_encode_char(utf8proc_toupper(codepoint),
                                     reinterpret_cast<utf8proc_uint8_t *>(newString->characters.get()) + doff);
        off += state;
    }
    return newString;
}

extern "C" runtime::SimpleOptional<runtime::Integer> sStringFind(String *string, String* search) {
    auto end = string->characters.get() + string->count;
    auto pos = std::search(string->characters.get(), end, search->characters.get(), search->characters.get() + search->count);
    if (pos != end) {
        return pos - string->characters.get();
    }
    return runtime::NoValue;
}

extern "C" runtime::SimpleOptional<runtime::Integer> sStringFindFromIndex(String *string, String* search,
                                                                          runtime::Integer offset) {
    if (offset >= string->count) {
        return runtime::NoValue;
    }
    auto end = string->characters.get() + string->count;
    auto pos = std::search(string->characters.get() + offset, end, search->characters.get(),
                           search->characters.get() + search->count);
    if (pos != end) {
        return pos - string->characters.get();
    }
    return runtime::NoValue;
}

extern "C" s::Data* sStringToData(String *string) {
    auto data = s::Data::init();
    data->count = string->count;
    data->data = string->characters;
    string->characters.retain();
    return data;
}

extern "C" void sStringCodepoints(String *string, runtime::Callable<void, runtime::Integer, runtime::Integer> cb) {
    for (size_t off = 0; off < string->count;) {
        utf8proc_int32_t codepoint;
        auto state = utf8proc_iterate(reinterpret_cast<utf8proc_uint8_t *>(string->characters.get()) + off,
                                      string->count, &codepoint);
        if (state < 0) break;
        cb(codepoint, off);
        off += state;
    }
}

extern "C" s::String* sStringTrim(String *string) {
    size_t begin = 0;

    for (; begin < string->count;) {
        utf8proc_int32_t codepoint;
        auto state = utf8proc_iterate(reinterpret_cast<utf8proc_uint8_t *>(string->characters.get()) + begin,
                                      string->count, &codepoint);
        if (state < 0) break;
        if (utf8proc_get_property(codepoint)->bidi_class != UTF8PROC_BIDI_CLASS_WS) break;
        begin += state;
    }

    size_t end = begin - 1;
    for (size_t i = begin; i < string->count;) {
        utf8proc_int32_t codepoint;
        auto state = utf8proc_iterate(reinterpret_cast<utf8proc_uint8_t *>(string->characters.get()) + i,
                                      string->count, &codepoint);
        if (state < 0) break;
        if (utf8proc_get_property(codepoint)->bidi_class != UTF8PROC_BIDI_CLASS_WS) {
            end = i;
        }
        i += state;
    }

    auto newString = String::init();
    newString->count = end - begin + 1;
    newString->characters = runtime::allocate<char>(string->count);
    std::memcpy(newString->characters.get(), string->characters.get() + begin, newString->count);
    return newString;
}

extern "C" void sStringGraphemes(String *string, runtime::Callable<void, s::String*> cb) {
    auto bytes = reinterpret_cast<utf8proc_uint8_t *>(string->characters.get());
    utf8proc_int32_t state = 0;
    utf8proc_int32_t prev;

    if (string->count == 0) {
        return;
    }

    size_t lastCut = 0;
    size_t off = utf8proc_iterate(bytes, string->count, &prev);

    while (off < string->count) {
        utf8proc_int32_t cp;
        auto c = utf8proc_iterate(bytes + off, string->count, &cp);

        if (utf8proc_grapheme_break_stateful(prev, cp, &state)) {
            auto newString = String::init();
            newString->count = off - lastCut;
            newString->characters = runtime::allocate<char>(newString->count);
            std::memcpy(newString->characters.get(), string->characters.get() + lastCut, newString->count);
            lastCut = off;
            cb(newString);
            newString->release();
        }
        prev = cp;
        off += c;
    }

    auto newString = String::init();
    newString->count = off - lastCut;
    newString->characters = runtime::allocate<char>(newString->count);
    std::memcpy(newString->characters.get(), string->characters.get() + lastCut, newString->count);
    lastCut = off;
    cb(newString);
    newString->release();
}

extern "C" s::String* sStringGraphemeSubstring(String *string, runtime::Integer from, runtime::Integer length) {
    auto bytes = reinterpret_cast<utf8proc_uint8_t *>(string->characters.get());
    utf8proc_int32_t state = 0;
    utf8proc_int32_t prev, cp;
    size_t beginCut = 0, off = utf8proc_iterate(bytes, string->count, &prev);

    if (length == 0) {
        auto newString = String::init();
        newString->count = 0;
        newString->characters = runtime::allocate<char>(0);
        return newString;
    }

    while (off < string->count && from > 0) {
        auto c = utf8proc_iterate(bytes + off, string->count, &cp);

        if (utf8proc_grapheme_break_stateful(prev, cp, &state) && --from == 0) {
            beginCut = off;
            prev = cp;
            off += c;
            break;
        }
        prev = cp;
        off += c;
    }
    while (off < string->count) {
        auto c = utf8proc_iterate(bytes + off, string->count, &cp);

        if (utf8proc_grapheme_break_stateful(prev, cp, &state) && --length == 0) {
            break;
        }
        prev = cp;
        off += c;
    }

    auto newString = String::init();
    newString->count = off - beginCut;
    newString->characters = runtime::allocate<char>(newString->count);
    std::memcpy(newString->characters.get(), string->characters.get() + beginCut, newString->count);
    return newString;
}

runtime::SimpleOptional<runtime::Integer> sStringToIntLength(const char *characters,
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
    return sStringToIntLength(string->characters.get(), string->count, base);
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
            auto exponent = sStringToIntLength(string->characters.get() + i + 1, string->count - i - 1, 10);
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
    auto len = string->count;
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    unsigned int h = runtime::internal::seed ^ len;

    const unsigned char * data = reinterpret_cast<const unsigned char *>(string->characters.get());

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
