//
//  String.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <ctype.h>
#include <cstring>
#include <cmath>
#include <utility>
#include "EmojicodeString.h"
#include "../utf8.h"
#include "EmojicodeList.h"
#include "algorithms.h"
#include "Thread.hpp"

EmojicodeInteger stringCompare(String *a, String *b) {
    if (a == b) {
        return 0;
    }
    if (a->length != b->length) {
        return a->length - b->length;
    }

    return memcmp(a->characters->value, b->characters->value, a->length * sizeof(EmojicodeChar));
}

bool stringEqual(String *a, String *b) {
    return stringCompare(a, b) == 0;
}

/** @warning GC-invoking */
Object* stringSubstring(EmojicodeInteger from, EmojicodeInteger length, Thread *thread) {
    String *string = static_cast<String *>(thread->getThisObject()->value);
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
    String *ostr = static_cast<String *>(ostro->value);

    ostr->length = length;
    ostr->characters = co;

    memcpy(ostr->characters->value, characters(static_cast<String *>(thread->getThisObject()->value)) + from,
           length * sizeof(EmojicodeChar));

    thread->release(1);
    return ostro;
}

char* stringToChar(String *str) {
    // Size needed for UTF8 representation
    size_t ds = u8_codingsize(characters(str), str->length);
    // Allocate space for the UTF8 string
    char *utf8str = new char[ds + 1];
    // Convert
    size_t written = u8_toutf8(utf8str, ds, characters(str), str->length);
    utf8str[written] = 0;
    return utf8str;
}

Object* stringFromChar(const char *cstring) {
    EmojicodeInteger len = u8_strlen(cstring);

    if (len == 0) {
        return emptyString;
    }

    Object *stro = newObject(CL_STRING);
    String *string = static_cast<String *>(stro->value);
    string->length = len;
    string->characters = newArray(len * sizeof(EmojicodeChar));

    u8_toucs(characters(string), len, cstring, strlen(cstring));

    return stro;
}

void stringPrintStdoutBrigde(Thread *thread, Value *destination) {
    String *string = static_cast<String *>(thread->getThisObject()->value);
    char *utf8str = stringToChar(string);
    printf("%s\n", utf8str);
    delete [] utf8str;
}

void stringEqualBridge(Thread *thread, Value *destination) {
    String *a = static_cast<String *>(thread->getThisObject()->value);
    String *b = static_cast<String *>(thread->getVariable(0).object->value);
    destination->raw = stringEqual(a, b);
}

void stringSubstringBridge(Thread *thread, Value *destination) {
    EmojicodeInteger from = thread->getVariable(0).raw;
    EmojicodeInteger length = thread->getVariable(1).raw;
    String *string = static_cast<String *>(thread->getThisObject()->value);

    if (from < 0) {
        from = (EmojicodeInteger)string->length + from;
    }
    if (length < 0) {
        length = (EmojicodeInteger)string->length + length;
    }

    destination->object = stringSubstring(from, length, thread);
}

void stringIndexOf(Thread *thread, Value *destination) {
    String *string = static_cast<String *>(thread->getThisObject()->value);
    String *search = static_cast<String *>(thread->getVariable(0).object->value);

    const void *location = findBytesInBytes(characters(string), string->length * sizeof(EmojicodeChar),
                                            characters(search), search->length * sizeof(EmojicodeChar));
    if (!location) {
        destination->makeNothingness();
    }
    else {
        destination->optionalSet(static_cast<EmojicodeInteger>(((Byte *)location - (Byte *)characters(string)) / sizeof(EmojicodeChar)));
    }
}

void stringTrimBridge(Thread *thread, Value *destination) {
    String *string = static_cast<String *>(thread->getThisObject()->value);

    EmojicodeInteger start = 0;
    EmojicodeInteger stop = string->length - 1;

    while (start < string->length && isWhitespace(characters(string)[start]))
        start++;

    while (stop > 0 && isWhitespace(characters(string)[stop]))
        stop--;

    destination->object = stringSubstring(start, stop - start + 1, thread);
}

void stringGetInput(Thread *thread, Value *destination) {
    String *prompt = static_cast<String *>(thread->getVariable(0).object->value);
    char *utf8str = stringToChar(prompt);
    printf("%s\n", utf8str);
    fflush(stdout);
    delete [] utf8str;

    int bufferSize = 50, oldBufferSize = 0;
    Object *buffer = newArray(bufferSize);
    size_t bufferUsedSize = 0;

    while (true) {
        fgets((char *)buffer->value + oldBufferSize, bufferSize - oldBufferSize, stdin);

        bufferUsedSize = strlen(static_cast<char *>(buffer->value));

        if (bufferUsedSize < bufferSize - 1) {
            if (((char *)buffer->value)[bufferUsedSize - 1] == '\n') {
                bufferUsedSize -= 1;
            }
            break;
        }

        oldBufferSize = bufferSize - 1;
        bufferSize *= 2;
        buffer = resizeArray(buffer, bufferSize);
    }

    EmojicodeInteger len = u8_strlen_l(static_cast<char *>(buffer->value), bufferUsedSize);

    String *string = static_cast<String *>(thread->getThisObject()->value);
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = static_cast<String *>(thread->getThisObject()->value);
    string->characters = chars;

    u8_toucs(characters(string), len, static_cast<char *>(buffer->value), bufferUsedSize);
}

void stringSplitByStringBridge(Thread *thread, Value *destination) {
    Object *const &listObject = thread->retain(newObject(CL_LIST));

    EmojicodeInteger firstOfSeperator = 0, seperatorIndex = 0, firstAfterSeperator = 0;

    for (EmojicodeInteger i = 0, l = ((String *)thread->getThisObject()->value)->length; i < l; i++) {
        Object *stringObject = thread->getThisObject();
        Object *separatorObject = thread->getVariable(0).object;
        String *separator = (String *)separatorObject->value;
        if (characters((String *)stringObject->value)[i] == characters(separator)[seperatorIndex]) {
            if (seperatorIndex == 0) {
                firstOfSeperator = i;
            }

            // Full seperator found
            if (firstOfSeperator + separator->length == i + 1) {
                Object *stro;
                if (firstAfterSeperator == firstOfSeperator) {
                    stro = emptyString;
                }
                else {
                    stro = stringSubstring(firstAfterSeperator, i - firstAfterSeperator - separator->length + 1, thread);
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
    String *string = (String *)stringObject->value;
    listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringSubstring(firstAfterSeperator, string->length - firstAfterSeperator, thread));

    Value list = thread->getVariable(1);
    thread->release(1);
    *destination = list;
}

void stringLengthBridge(Thread *thread, Value *destination) {
    String *string = static_cast<String *>(thread->getThisObject()->value);
    destination->raw = string->length;
}

void stringUTF8LengthBridge(Thread *thread, Value *destination) {
    String *str = static_cast<String *>(thread->getThisObject()->value);
    destination->raw = u8_codingsize(characters(str), str->length);
}

void stringByAppendingSymbolBridge(Thread *thread, Value *destination) {
    String *string = static_cast<String *>(thread->getThisObject()->value);
    Object *const &co = thread->retain(newArray((string->length + 1) * sizeof(EmojicodeChar)));

    Object *ostro = newObject(CL_STRING);
    String *ostr = static_cast<String *>(ostro->value);
    string = static_cast<String *>(thread->getThisObject()->value);

    ostr->length = string->length + 1;
    ostr->characters = co;

    memcpy(characters(ostr), characters(string), string->length * sizeof(EmojicodeChar));

    characters(ostr)[string->length] = thread->getVariable(0).character;

    destination->object = ostro;
    thread->release(1);
}

void stringSymbolAtBridge(Thread *thread, Value *destination) {
    EmojicodeInteger index = thread->getVariable(0).raw;
    String *str = static_cast<String *>(thread->getThisObject()->value);
    if (index >= str->length) {
        destination->makeNothingness();
        return;
    }

    destination->optionalSet(characters(str)[index]);
}

void stringBeginsWithBridge(Thread *thread, Value *destination) {
    String *a = static_cast<String *>(thread->getThisObject()->value);
    String *with = static_cast<String *>(thread->getVariable(0).object->value);
    if (a->length < with->length) {
        destination->raw = 0;
        return;
    }

    destination->raw = memcmp(a->characters->value, with->characters->value,
                              with->length * sizeof(EmojicodeChar)) == 0 ? 1 : 0;
}

void stringEndsWithBridge(Thread *thread, Value *destination) {
    String *a = static_cast<String *>(thread->getThisObject()->value);
    String *end = static_cast<String *>(thread->getVariable(0).object->value);
    if (a->length < end->length) {
        destination->raw = 0;
        return;
    }

    destination->raw = memcmp(((EmojicodeChar*)a->characters->value) + (a->length - end->length),
                              end->characters->value, end->length * sizeof(EmojicodeChar)) == 0 ? 1 : 0;
}

void stringSplitBySymbolBridge(Thread *thread, Value *destination) {
    EmojicodeChar separator = thread->getVariable(0).character;
    Object *const &list = thread->retain(newObject(CL_LIST));

    EmojicodeInteger from = 0;

    for (EmojicodeInteger i = 0, l = ((String *)thread->getThisObject()->value)->length; i < l; i++) {
        if (characters((String *)thread->getThisObject()->value)[i] == separator) {
            listAppendDestination(list, thread)->copySingleValue(T_OBJECT, stringSubstring(from, i - from, thread));
            from = i + 1;
        }
    }

    Object *stringObject = thread->getThisObject();
    listAppendDestination(list, thread)->copySingleValue(T_OBJECT,
                                    stringSubstring(from, ((String *) stringObject->value)->length - from, thread));

    thread->release(1);
    destination->object = list;
}

void stringToData(Thread *thread, Value *destination) {
    String *str = static_cast<String *>(thread->getThisObject()->value);

    size_t ds = u8_codingsize(characters(str), str->length);

    Object *const &bytesObject = thread->retain(newArray(ds));

    str = static_cast<String *>(thread->getThisObject()->value);
    u8_toutf8(static_cast<char *>(bytesObject->value), ds, characters(str), str->length);

    Object *o = newObject(CL_DATA);
    Data *d = static_cast<Data *>(o->value);
    d->length = ds;
    d->bytesObject = bytesObject;
    d->bytes = static_cast<char *>(d->bytesObject->value);

    thread->release(1);
    destination->object = o;
}

void stringToCharacterList(Thread *thread, Value *destination) {
    String *str = static_cast<String *>(thread->getThisObject()->value);
    Object *list = newObject(CL_LIST);

    for (size_t i = 0; i < str->length; i++) {
        listAppendDestination(list, thread)->copySingleValue(T_SYMBOL, characters(str)[i]);
    }
    destination->object = list;
}

void stringJSON(Thread *thread, Value *destination) {
    parseJSON(thread, reinterpret_cast<Box *>(destination));
}

void initStringFromSymbolList(String *str, List *list) {
    size_t count = list->count;
    str->length = count;
    str->characters = newArray(count * sizeof(EmojicodeChar));

    for (size_t i = 0; i < count; i++) {
        Box b = list->elements()[i];
        if (b.isNothingness()) break;
        characters(str)[i] = b.value1.character;
    }
}

void stringFromSymbolListBridge(Thread *thread, Value *destination) {
    String *str = static_cast<String *>(thread->getThisObject()->value);
    List *list = static_cast<List *>(thread->getVariable(0).object->value);

    initStringFromSymbolList(str, list);
}

void stringFromStringList(Thread *thread, Value *destination) {
    size_t stringSize = 0;
    size_t appendLocation = 0;

    {
        List *list = static_cast<List *>(thread->getVariable(0).object->value);
        String *glue = static_cast<String *>(thread->getVariable(1).object->value);

        for (size_t i = 0; i < list->count; i++) {
            stringSize += ((String *)list->elements()[i].value1.object->value)->length;
        }

        if (list->count > 0) {
            stringSize += glue->length * (list->count - 1);
        }
    }

    Object *co = newArray(stringSize * sizeof(EmojicodeChar));

    {
        List *list = static_cast<List *>(thread->getVariable(0).object->value);
        String *glue = static_cast<String *>(thread->getVariable(1).object->value);

        String *string = static_cast<String *>(thread->getThisObject()->value);
        string->length = stringSize;
        string->characters = co;

        for (size_t i = 0; i < list->count; i++) {
            String *aString = static_cast<String *>(list->elements()[i].value1.object->value);
            memcpy(characters(string) + appendLocation, characters(aString), aString->length * sizeof(EmojicodeChar));
            appendLocation += aString->length;
            if (i + 1 < list->count) {
                memcpy(characters(string) + appendLocation, characters(glue), glue->length * sizeof(EmojicodeChar));
                appendLocation += glue->length;
            }
        }
    }
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
    String *string = (String *)thread->getThisObject()->value;

    auto pair = charactersToInteger(characters(string), base, string->length);
    if (pair.second) destination->optionalSet(pair.first);
    else destination->makeNothingness();
}

void stringToDouble(Thread *thread, Value *destination) {
    String *string = (String *)thread->getThisObject()->value;

    if (string->length == 0) {
        destination->makeNothingness();
        return;
    }

    EmojicodeChar *characters = characters(string);

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
    size_t length = static_cast<String *>(thread->getThisObject()->value)->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    String *news = static_cast<String *>(o->value);
    news->characters = characters;
    news->length = length;
    String *os = static_cast<String *>(thread->getThisObject()->value);
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = characters(os)[i];
        if (c <= 'z') characters(news)[i] = toupper(c);
        else characters(news)[i] = c;
    }
    thread->release(1);
    destination->object = o;
}

void stringToLowercase(Thread *thread, Value *destination) {
    Object *const &o = thread->retain(newObject(CL_STRING));
    size_t length = static_cast<String *>(thread->getThisObject()->value)->length;

    Object *characters = newArray(length * sizeof(EmojicodeChar));
    String *news = static_cast<String *>(o->value);
    news->characters = characters;
    news->length = length;
    String *os = static_cast<String *>(thread->getThisObject()->value);
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = characters(os)[i];
        if (c <= 'z') characters(news)[i] = tolower(c);
        else characters(news)[i] = c;
    }
    thread->release(1);
    destination->object = o;
}

void stringCompareBridge(Thread *thread, Value *destination) {
    String *a = static_cast<String *>(thread->getThisObject()->value);
    String *b = static_cast<String *>(thread->getVariable(0).object->value);
    destination->raw = stringCompare(a, b);
}

void stringMark(Object *self) {
    auto string = static_cast<String *>(self->value);
    if (string->characters) {
        mark(&string->characters);
    }
}
