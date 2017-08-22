//
//  String.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "String.hpp"
#include "../utf8.h"
#include "List.hpp"
#include "Data.hpp"
#include "Thread.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <utility>

namespace Emojicode {

EmojicodeInteger stringCompare(String *a, String *b) {
    if (a == b) {
        return 0;
    }
    if (a->length != b->length) {
        return a->length - b->length;
    }

    return std::memcmp(a->characters(), b->characters(), a->length * sizeof(EmojicodeChar));
}

bool stringEqual(String *a, String *b) {
    return stringCompare(a, b) == 0;
}

/** @warning GC-invoking */
Object* stringSubstring(EmojicodeInteger from, EmojicodeInteger length, Thread *thread) {
    auto *string = thread->thisObject()->val<String>();
    if (from >= string->length) {
        length = 0;
        from = 0;
    }

    if (length + from >= string->length) {
        length = string->length - from;
    }

    if (length == 0) {
        return emptyString;
    }

    auto co = thread->retain(newArray(length * sizeof(EmojicodeChar)));

    Object *ostro = newObject(CL_STRING);
    auto *ostr = ostro->val<String>();

    ostr->length = length;
    ostr->charactersObject = co.unretainedPointer();

    std::memcpy(ostr->characters(), thread->thisObject()->val<String>()->characters() + from,
                length * sizeof(EmojicodeChar));

    thread->release(1);
    return ostro;
}

const char* stringToCString(Object *str) {
    auto string = str->val<String>();
    size_t ds = u8_codingsize(string->characters(), string->length);
    auto *utf8str = newArray(ds + 1)->val<char>();
    // Convert
    size_t written = u8_toutf8(utf8str, ds, string->characters(), string->length);
    utf8str[written] = 0;
    return utf8str;
}

Object* stringFromChar(const char *cstring) {
    EmojicodeInteger len = u8_strlen(cstring);

    if (len == 0) {
        return emptyString;
    }

    Object *stro = newObject(CL_STRING);
    auto *string = stro->val<String>();
    string->length = len;
    string->charactersObject = newArray(len * sizeof(EmojicodeChar));

    u8_toucs(string->characters(), len, cstring, strlen(cstring));

    return stro;
}

void stringPrintStdoutBrigde(Thread *thread) {
    printf("%s\n", stringToCString(thread->thisObject()));
    thread->returnFromFunction();
}

void stringEqualBridge(Thread *thread) {
    auto *a = thread->thisObject()->val<String>();
    auto *b = thread->variable(0).object->val<String>();
    thread->returnFromFunction(stringEqual(a, b));
}

void stringSubstringBridge(Thread *thread) {
    EmojicodeInteger from = thread->variable(0).raw;
    EmojicodeInteger length = thread->variable(1).raw;
    auto *string = thread->thisObject()->val<String>();

    if (from < 0) {
        from = string->length + from;
    }
    if (length < 0) {
        length = string->length + length;
    }

    thread->returnFromFunction(stringSubstring(from, length, thread));
}

void stringIndexOf(Thread *thread) {
    auto *string = thread->thisObject()->val<String>();
    auto *search = thread->variable(0).object->val<String>();

    auto last = string->characters() + string->length;
    EmojicodeChar *location = std::search(string->characters(), last, search->characters(),
                                          search->characters() + search->length);

    if (location == last) {
        thread->returnNothingnessFromFunction();
    }
    else {
        thread->returnOEValueFromFunction(static_cast<EmojicodeInteger>(location - string->characters()));
    }
}

void stringTrimBridge(Thread *thread) {
    auto *string = thread->thisObject()->val<String>();

    EmojicodeInteger start = 0;
    EmojicodeInteger stop = string->length - 1;

    while (start < string->length && isWhitespace(string->characters()[start])) {
        start++;
    }

    while (stop > 0 && isWhitespace(string->characters()[stop])) {
        stop--;
    }

    thread->returnFromFunction(stringSubstring(start, stop - start + 1, thread));
}

void stringGetInput(Thread *thread) {
    printf("%s\n", stringToCString(thread->variable(0).object));
    fflush(stdout);

    int bufferSize = 50, oldBufferSize = 0;
    Object *buffer = newArray(bufferSize);
    size_t bufferUsedSize = 0;

    while (true) {
        fgets(buffer->val<char>() + oldBufferSize, bufferSize - oldBufferSize, stdin);

        bufferUsedSize = strlen(buffer->val<char>());

        if (bufferUsedSize < bufferSize - 1) {
            if (buffer->val<char>()[bufferUsedSize - 1] == '\n') {
                bufferUsedSize -= 1;
            }
            break;
        }

        oldBufferSize = bufferSize - 1;
        bufferSize *= 2;
        buffer = resizeArray(buffer, bufferSize, thread);
    }

    EmojicodeInteger len = u8_strlen_l(buffer->val<char>(), bufferUsedSize);

    auto *string = thread->thisObject()->val<String>();
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = thread->thisObject()->val<String>();
    string->charactersObject = chars;

    u8_toucs(string->characters(), len, buffer->val<char>(), bufferUsedSize);
    thread->returnFromFunction(thread->thisContext());
}

void stringSplitByStringBridge(Thread *thread) {
    auto listObject = thread->retain(newObject(CL_LIST));

    EmojicodeInteger firstOfSeperator = 0, seperatorIndex = 0, firstAfterSeperator = 0;

    for (EmojicodeInteger i = 0, l = thread->thisObject()->val<String>()->length; i < l; i++) {
        Object *stringObject = thread->thisObject();
        Object *separator = thread->variable(0).object;
        if (stringObject->val<String>()->characters()[i] == separator->val<String>()->characters()[seperatorIndex]) {
            if (seperatorIndex == 0) {
                firstOfSeperator = i;
            }

            // Full seperator found
            if (firstOfSeperator + separator->val<String>()->length == i + 1) {
                Object *stro;
                if (firstAfterSeperator == firstOfSeperator) {
                    stro = emptyString;
                }
                else {
                    stro = stringSubstring(firstAfterSeperator,
                                           i - firstAfterSeperator - separator->val<String>()->length + 1, thread);
                }
                listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stro);
                seperatorIndex = 0;
                firstAfterSeperator = i + 1;
            }
            else {
                seperatorIndex++;
            }
        }
        else if (seperatorIndex > 0) {  // and not matching
            seperatorIndex = 0;
            // We search from the begin of the last partial match
            i = firstOfSeperator;
        }
    }

    Object *stringObject = thread->thisObject();
    auto *string = stringObject->val<String>();
    listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringSubstring(firstAfterSeperator, string->length - firstAfterSeperator, thread));

    thread->release(1);
    thread->returnFromFunction(listObject.unretainedPointer());
}

void stringLengthBridge(Thread *thread) {
    thread->returnFromFunction(thread->thisObject()->val<String>()->length);
}

void stringUTF8LengthBridge(Thread *thread) {
    auto *str = thread->thisObject()->val<String>();
    thread->returnFromFunction(static_cast<EmojicodeInteger>(u8_codingsize(str->characters(), str->length)));
}

void stringByAppendingSymbolBridge(Thread *thread) {
    auto *string = thread->thisObject()->val<String>();
    auto co = thread->retain(newArray((string->length + 1) * sizeof(EmojicodeChar)));

    Object *ostro = newObject(CL_STRING);
    auto *ostr = ostro->val<String>();
    string = thread->thisObject()->val<String>();

    ostr->length = string->length + 1;
    ostr->charactersObject = co.unretainedPointer();

    std::memcpy(ostr->characters(), string->characters(), string->length * sizeof(EmojicodeChar));

    ostr->characters()[string->length] = thread->variable(0).character;

    thread->release(1);
    thread->returnFromFunction(ostro);
}

void stringSymbolAtBridge(Thread *thread) {
    EmojicodeInteger index = thread->variable(0).raw;
    auto *str = thread->thisObject()->val<String>();
    if (index >= str->length) {
        thread->returnNothingnessFromFunction();
        return;
    }

    thread->returnOEValueFromFunction(str->characters()[index]);
}

void stringBeginsWithBridge(Thread *thread) {
    auto *a = thread->thisObject()->val<String>();
    auto *with = thread->variable(0).object->val<String>();
    if (a->length < with->length) {
        thread->returnFromFunction(false);
        return;
    }

    thread->returnFromFunction(std::memcmp(a->characters(), with->characters(),
                                           with->length * sizeof(EmojicodeChar)) == 0);
}

void stringEndsWithBridge(Thread *thread) {
    auto *a = thread->thisObject()->val<String>();
    auto *end = thread->variable(0).object->val<String>();
    if (a->length < end->length) {
        thread->returnFromFunction(false);
        return;
    }

    thread->returnFromFunction(std::memcmp(a->characters() + (a->length - end->length),
                                           end->characters(), end->length * sizeof(EmojicodeChar)) == 0);
}

void stringSplitBySymbolBridge(Thread *thread) {
    EmojicodeChar separator = thread->variable(0).character;
    auto list = thread->retain(newObject(CL_LIST));

    EmojicodeInteger from = 0;

    for (EmojicodeInteger i = 0, l = thread->thisObject()->val<String>()->length; i < l; i++) {
        if (thread->thisObject()->val<String>()->characters()[i] == separator) {
            listAppendDestination(list, thread)->copySingleValue(T_OBJECT, stringSubstring(from, i - from, thread));
            from = i + 1;
        }
    }

    Object *stringObject = thread->thisObject();
    listAppendDestination(list, thread)->copySingleValue(T_OBJECT,
                                    stringSubstring(from, stringObject->val<String>()->length - from, thread));

    thread->release(1);
    thread->returnFromFunction(list.unretainedPointer());
}

void stringToData(Thread *thread) {
    auto *str = thread->thisObject()->val<String>();

    size_t ds = u8_codingsize(str->characters(), str->length);

    auto bytesObject = thread->retain(newArray(ds));

    str = thread->thisObject()->val<String>();
    u8_toutf8(bytesObject->val<char>(), ds, str->characters(), str->length);

    Object *o = newObject(CL_DATA);
    auto *d = o->val<Data>();
    d->length = ds;
    d->bytesObject = bytesObject.unretainedPointer();
    d->bytes = d->bytesObject->val<char>();

    thread->release(1);
    thread->returnFromFunction(o);
}

void stringToCharacterList(Thread *thread) {
    auto *str = thread->thisObject()->val<String>();
    auto list = thread->retain(newObject(CL_LIST));

    for (size_t i = 0; i < str->length; i++) {
        listAppendDestination(list, thread)->copySingleValue(T_SYMBOL, str->characters()[i]);
    }

    thread->returnFromFunction(list.unretainedPointer());
}

void initStringFromSymbolList(String *str, List *list) {
    size_t count = list->count;
    str->length = count;
    str->charactersObject = newArray(count * sizeof(EmojicodeChar));

    for (size_t i = 0; i < count; i++) {
        Box b = list->elements()[i];
        if (b.isNothingness()) {
            break;
        }
        str->characters()[i] = b.value1.character;
    }
}

void stringFromSymbolListBridge(Thread *thread) {
    auto *str = thread->thisObject()->val<String>();
    initStringFromSymbolList(str, thread->variable(0).object->val<List>());
    thread->returnFromFunction(thread->thisContext());
}

void stringFromStringList(Thread *thread) {
    size_t stringSize = 0;
    size_t appendLocation = 0;

    {
        auto *list = thread->variable(0).object->val<List>();
        auto *glue = thread->variable(1).object->val<String>();

        for (size_t i = 0; i < list->count; i++) {
            stringSize += list->elements()[i].value1.object->val<String>()->length;
        }

        if (list->count > 0) {
            stringSize += glue->length * (list->count - 1);
        }
    }

    Object *co = newArray(stringSize * sizeof(EmojicodeChar));

    {
        auto *list = thread->variable(0).object->val<List>();
        auto *glue = thread->variable(1).object->val<String>();

        auto *string = thread->thisObject()->val<String>();
        string->length = stringSize;
        string->charactersObject = co;

        for (size_t i = 0; i < list->count; i++) {
            auto *aString = list->elements()[i].value1.object->val<String>();
            std::memcpy(string->characters() + appendLocation, aString->characters(),
                        aString->length * sizeof(EmojicodeChar));
            appendLocation += aString->length;
            if (i + 1 < list->count) {
                std::memcpy(string->characters() + appendLocation, glue->characters(),
                            glue->length * sizeof(EmojicodeChar));
                appendLocation += glue->length;
            }
        }
    }
    thread->returnFromFunction(thread->thisContext());
}

std::pair<EmojicodeInteger, bool> charactersToInteger(const EmojicodeChar *characters, EmojicodeInteger base,
                                                      EmojicodeInteger length) {
    if (length == 0) {
        return std::make_pair(0, false);
    }
    EmojicodeInteger x = 0;
    for (size_t i = 0; i < length; i++) {
        if (i == 0 && (characters[i] == '-' || characters[i] == '+')) {
            if (length < 2) {
                return std::make_pair(0, false);
            }
            continue;
        }

        EmojicodeInteger b = base;
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
            return std::make_pair(0, false);
        }

        x *= base;
        x += b;
    }

    if (characters[0] == '-') {
        x *= -1;
    }
    return std::make_pair(x, true);
}

void stringToInteger(Thread *thread) {
    EmojicodeInteger base = thread->variable(0).raw;
    auto *string = thread->thisObject()->val<String>();

    auto pair = charactersToInteger(string->characters(), base, string->length);
    if (pair.second) {
        thread->returnOEValueFromFunction(pair.first);
    }
    else {
        thread->returnNothingnessFromFunction();
    }
}

void stringToDouble(Thread *thread) {
    auto *string = thread->thisObject()->val<String>();

    if (string->length == 0) {
        thread->returnNothingnessFromFunction();
        return;
    }

    EmojicodeChar *characters = string->characters();

    double d = 0.0;
    bool sign = true;
    bool foundSeparator = false;
    bool foundDigit = false;
    size_t i = 0, decimalPlace = 0;

    if (characters[0] == '-') {
        sign = false;
        i++;
    }
    else if (characters[0] == '+') {
        i++;
    }

    for (; i < string->length; i++) {
        if (characters[i] == '.') {
            if (foundSeparator) {
                thread->returnNothingnessFromFunction();
                return;
            }
            foundSeparator = true;
            continue;
        }
        if (characters[i] == 'e' || characters[i] == 'E') {
            auto exponent = charactersToInteger(characters + i + 1, 10, string->length - i - 1);
            if (!exponent.second) {
                thread->returnNothingnessFromFunction();
                return;
            }
            d *= pow(10, exponent.first);
            break;
        }
        if ('0' <= characters[i] && characters[i] <= '9') {
            d *= 10;
            d += characters[i] - '0';
            if (foundSeparator) {
                decimalPlace++;
            }
            foundDigit = true;
        } else {
            thread->returnNothingnessFromFunction();
            return;
        }
    }

    if (!foundDigit) {
        thread->returnNothingnessFromFunction();
        return;
    }

    d /= pow(10, decimalPlace);

    if (!sign) {
        d *= -1;
    }
    thread->returnOEValueFromFunction(d);
}

void stringToUppercase(Thread *thread) {
    auto o = thread->retain(newObject(CL_STRING));
    size_t length = thread->thisObject()->val<String>()->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    auto *news = o->val<String>();
    news->charactersObject = characters;
    news->length = length;
    auto *os = thread->thisObject()->val<String>();
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = os->characters()[i];
        if (c <= 'z') {
            news->characters()[i] = toupper(c);
        }
        else {
            news->characters()[i] = c;
        }
    }
    thread->release(1);
    thread->returnFromFunction(o.unretainedPointer());
}

void stringToLowercase(Thread *thread) {
    auto o = thread->retain(newObject(CL_STRING));
    size_t length = thread->thisObject()->val<String>()->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    auto *news = o->val<String>();
    news->charactersObject = characters;
    news->length = length;
    auto *os = thread->thisObject()->val<String>();
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = os->characters()[i];
        if (c <= 'z') {
            news->characters()[i] = tolower(c);
        }
        else {
            news->characters()[i] = c;
        }
    }
    thread->release(1);
    thread->returnFromFunction(o.unretainedPointer());
}

void stringCompareBridge(Thread *thread) {
    auto *a = thread->thisObject()->val<String>();
    auto *b = thread->variable(0).object->val<String>();
    thread->returnFromFunction(stringCompare(a, b));
}

void stringMark(Object *self) {
    auto string = self->val<String>();
    if (string->charactersObject != nullptr) {
        mark(&string->charactersObject);
    }
}

}  // namespace Emojicode
