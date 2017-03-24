//
//  String.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "String.h"
#include "../utf8.h"
#include "List.h"
#include "Thread.hpp"
#include "standard.h"
#include <ctype.h>
#include <cstring>
#include <cmath>
#include <utility>
#include <algorithm>

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
    String *string = thread->getThisObject()->val<String>();
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

    Object *const &co = thread->retain(newArray(length * sizeof(EmojicodeChar)));

    Object *ostro = newObject(CL_STRING);
    String *ostr = ostro->val<String>();

    ostr->length = length;
    ostr->charactersObject = co;

    std::memcpy(ostr->characters(), thread->getThisObject()->val<String>()->characters() + from,
                length * sizeof(EmojicodeChar));

    thread->release(1);
    return ostro;
}

const char* stringToCString(Object *str) {
    auto string = str->val<String>();
    size_t ds = u8_codingsize(string->characters(), string->length);
    char *utf8str = newArray(ds + 1)->val<char>();
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
    String *string = stro->val<String>();
    string->length = len;
    string->charactersObject = newArray(len * sizeof(EmojicodeChar));

    u8_toucs(string->characters(), len, cstring, strlen(cstring));

    return stro;
}

void stringPrintStdoutBrigde(Thread *thread, Value *destination) {
    printf("%s\n", stringToCString(thread->getThisObject()));
}

void stringEqualBridge(Thread *thread, Value *destination) {
    String *a = thread->getThisObject()->val<String>();
    String *b = thread->getVariable(0).object->val<String>();
    destination->raw = stringEqual(a, b);
}

void stringSubstringBridge(Thread *thread, Value *destination) {
    EmojicodeInteger from = thread->getVariable(0).raw;
    EmojicodeInteger length = thread->getVariable(1).raw;
    String *string = thread->getThisObject()->val<String>();

    if (from < 0) {
        from = (EmojicodeInteger)string->length + from;
    }
    if (length < 0) {
        length = (EmojicodeInteger)string->length + length;
    }

    destination->object = stringSubstring(from, length, thread);
}

void stringIndexOf(Thread *thread, Value *destination) {
    String *string = thread->getThisObject()->val<String>();
    String *search = thread->getVariable(0).object->val<String>();

    auto last = string->characters() + string->length;
    EmojicodeChar *location = std::search(string->characters(), last, search->characters(),
                                          search->characters() + search->length);

    if (location == last) {
        destination->makeNothingness();
    }
    else {
        destination->optionalSet(static_cast<EmojicodeInteger>(location - string->characters()));
    }
}

void stringTrimBridge(Thread *thread, Value *destination) {
    String *string = thread->getThisObject()->val<String>();

    EmojicodeInteger start = 0;
    EmojicodeInteger stop = string->length - 1;

    while (start < string->length && isWhitespace(string->characters()[start]))
        start++;

    while (stop > 0 && isWhitespace(string->characters()[stop]))
        stop--;

    destination->object = stringSubstring(start, stop - start + 1, thread);
}

void stringGetInput(Thread *thread, Value *destination) {
    printf("%s\n", stringToCString(thread->getVariable(0).object));
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
        buffer = resizeArray(buffer, bufferSize);
    }

    EmojicodeInteger len = u8_strlen_l(buffer->val<char>(), bufferUsedSize);

    String *string = thread->getThisObject()->val<String>();
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = thread->getThisObject()->val<String>();
    string->charactersObject = chars;

    u8_toucs(string->characters(), len, buffer->val<char>(), bufferUsedSize);
    *destination = thread->getThisContext();
}

void stringSplitByStringBridge(Thread *thread, Value *destination) {
    Object *const &listObject = thread->retain(newObject(CL_LIST));

    EmojicodeInteger firstOfSeperator = 0, seperatorIndex = 0, firstAfterSeperator = 0;

    for (EmojicodeInteger i = 0, l = thread->getThisObject()->val<String>()->length; i < l; i++) {
        Object *stringObject = thread->getThisObject();
        Object *separator = thread->getVariable(0).object;
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

    Object *stringObject = thread->getThisObject();
    String *string = stringObject->val<String>();
    listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringSubstring(firstAfterSeperator, string->length - firstAfterSeperator, thread));

    *destination = listObject;
    thread->release(1);
}

void stringLengthBridge(Thread *thread, Value *destination) {
    destination->raw = thread->getThisObject()->val<String>()->length;
}

void stringUTF8LengthBridge(Thread *thread, Value *destination) {
    String *str = thread->getThisObject()->val<String>();
    destination->raw = u8_codingsize(str->characters(), str->length);
}

void stringByAppendingSymbolBridge(Thread *thread, Value *destination) {
    String *string = thread->getThisObject()->val<String>();
    Object *const &co = thread->retain(newArray((string->length + 1) * sizeof(EmojicodeChar)));

    Object *ostro = newObject(CL_STRING);
    String *ostr = ostro->val<String>();
    string = thread->getThisObject()->val<String>();

    ostr->length = string->length + 1;
    ostr->charactersObject = co;

    std::memcpy(ostr->characters(), string->characters(), string->length * sizeof(EmojicodeChar));

    ostr->characters()[string->length] = thread->getVariable(0).character;

    destination->object = ostro;
    thread->release(1);
}

void stringSymbolAtBridge(Thread *thread, Value *destination) {
    EmojicodeInteger index = thread->getVariable(0).raw;
    String *str = thread->getThisObject()->val<String>();
    if (index >= str->length) {
        destination->makeNothingness();
        return;
    }

    destination->optionalSet(str->characters()[index]);
}

void stringBeginsWithBridge(Thread *thread, Value *destination) {
    String *a = thread->getThisObject()->val<String>();
    String *with = thread->getVariable(0).object->val<String>();
    if (a->length < with->length) {
        destination->raw = 0;
        return;
    }

    destination->raw = std::memcmp(a->characters(), with->characters(), with->length * sizeof(EmojicodeChar)) == 0;
}

void stringEndsWithBridge(Thread *thread, Value *destination) {
    String *a = thread->getThisObject()->val<String>();
    String *end = thread->getVariable(0).object->val<String>();
    if (a->length < end->length) {
        destination->raw = 0;
        return;
    }

    destination->raw = std::memcmp(a->characters() + (a->length - end->length),
                                   end->characters(), end->length * sizeof(EmojicodeChar)) == 0;
}

void stringSplitBySymbolBridge(Thread *thread, Value *destination) {
    EmojicodeChar separator = thread->getVariable(0).character;
    Object *const &list = thread->retain(newObject(CL_LIST));

    EmojicodeInteger from = 0;

    for (EmojicodeInteger i = 0, l = thread->getThisObject()->val<String>()->length; i < l; i++) {
        if (thread->getThisObject()->val<String>()->characters()[i] == separator) {
            listAppendDestination(list, thread)->copySingleValue(T_OBJECT, stringSubstring(from, i - from, thread));
            from = i + 1;
        }
    }

    Object *stringObject = thread->getThisObject();
    listAppendDestination(list, thread)->copySingleValue(T_OBJECT,
                                    stringSubstring(from, stringObject->val<String>()->length - from, thread));

    thread->release(1);
    destination->object = list;
}

void stringToData(Thread *thread, Value *destination) {
    String *str = thread->getThisObject()->val<String>();

    size_t ds = u8_codingsize(str->characters(), str->length);

    Object *const &bytesObject = thread->retain(newArray(ds));

    str = thread->getThisObject()->val<String>();
    u8_toutf8(bytesObject->val<char>(), ds, str->characters(), str->length);

    Object *o = newObject(CL_DATA);
    Data *d = o->val<Data>();
    d->length = ds;
    d->bytesObject = bytesObject;
    d->bytes = d->bytesObject->val<char>();

    thread->release(1);
    destination->object = o;
}

void stringToCharacterList(Thread *thread, Value *destination) {
    String *str = thread->getThisObject()->val<String>();
    Object *list = newObject(CL_LIST);

    for (size_t i = 0; i < str->length; i++) {
        listAppendDestination(list, thread)->copySingleValue(T_SYMBOL, str->characters()[i]);
    }
    destination->object = list;
}

void stringJSON(Thread *thread, Value *destination) {
    parseJSON(thread, reinterpret_cast<Box *>(destination));
}

void initStringFromSymbolList(String *str, List *list) {
    size_t count = list->count;
    str->length = count;
    str->charactersObject = newArray(count * sizeof(EmojicodeChar));

    for (size_t i = 0; i < count; i++) {
        Box b = list->elements()[i];
        if (b.isNothingness()) break;
        str->characters()[i] = b.value1.character;
    }
}

void stringFromSymbolListBridge(Thread *thread, Value *destination) {
    String *str = thread->getThisObject()->val<String>();
    initStringFromSymbolList(str, thread->getVariable(0).object->val<List>());
    *destination = thread->getThisContext();
}

void stringFromStringList(Thread *thread, Value *destination) {
    size_t stringSize = 0;
    size_t appendLocation = 0;

    {
        List *list = thread->getVariable(0).object->val<List>();
        String *glue = thread->getVariable(1).object->val<String>();

        for (size_t i = 0; i < list->count; i++) {
            stringSize += list->elements()[i].value1.object->val<String>()->length;
        }

        if (list->count > 0) {
            stringSize += glue->length * (list->count - 1);
        }
    }

    Object *co = newArray(stringSize * sizeof(EmojicodeChar));

    {
        List *list = thread->getVariable(0).object->val<List>();
        String *glue = thread->getVariable(1).object->val<String>();

        String *string = thread->getThisObject()->val<String>();
        string->length = stringSize;
        string->charactersObject = co;

        for (size_t i = 0; i < list->count; i++) {
            String *aString = list->elements()[i].value1.object->val<String>();
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
    *destination = thread->getThisContext();
}

std::pair<EmojicodeInteger, bool> charactersToInteger(EmojicodeChar *characters, EmojicodeInteger base,
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

void stringToInteger(Thread *thread, Value *destination) {
    EmojicodeInteger base = thread->getVariable(0).raw;
    String *string = thread->getThisObject()->val<String>();

    auto pair = charactersToInteger(string->characters(), base, string->length);
    if (pair.second) destination->optionalSet(pair.first);
    else destination->makeNothingness();
}

void stringToDouble(Thread *thread, Value *destination) {
    String *string = thread->getThisObject()->val<String>();

    if (string->length == 0) {
        destination->makeNothingness();
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
    } else if (characters[0] == '+') {
        i++;
    }

    for (; i < string->length; i++) {
        if (characters[i] == '.') {
            if (foundSeparator) {
                destination->makeNothingness();
                return;
            } else {
                foundSeparator = true;
                continue;
            }
        }
        if (characters[i] == 'e' || characters[i] == 'E') {
            auto exponent = charactersToInteger(characters + i + 1, 10, string->length - i - 1);
            if (!exponent.second) {
                destination->makeNothingness();
                return;
            } else {
                d *= pow(10, exponent.first);
            }
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
            destination->makeNothingness();
            return;
        }
    }

    if (!foundDigit) {
        destination->makeNothingness();
        return;
    }

    d /= pow(10, decimalPlace);

    if (!sign) {
        d *= -1;
    }
    destination->optionalSet(d);
}

void stringToUppercase(Thread *thread, Value *destination) {
    Object *const &o = thread->retain(newObject(CL_STRING));
    size_t length = thread->getThisObject()->val<String>()->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    String *news = o->val<String>();
    news->charactersObject = characters;
    news->length = length;
    String *os = thread->getThisObject()->val<String>();
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = os->characters()[i];
        if (c <= 'z') news->characters()[i] = toupper(c);
        else news->characters()[i] = c;
    }
    thread->release(1);
    destination->object = o;
}

void stringToLowercase(Thread *thread, Value *destination) {
    Object *const &o = thread->retain(newObject(CL_STRING));
    size_t length = thread->getThisObject()->val<String>()->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    String *news = o->val<String>();
    news->charactersObject = characters;
    news->length = length;
    String *os = thread->getThisObject()->val<String>();
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = os->characters()[i];
        if (c <= 'z') news->characters()[i] = tolower(c);
        else news->characters()[i] = c;
    }
    thread->release(1);
    destination->object = o;
}

void stringCompareBridge(Thread *thread, Value *destination) {
    String *a = thread->getThisObject()->val<String>();
    String *b = thread->getVariable(0).object->val<String>();
    destination->raw = stringCompare(a, b);
}

void stringMark(Object *self) {
    auto string = self->val<String>();
    if (string->charactersObject) {
        mark(&string->charactersObject);
    }
}

}
