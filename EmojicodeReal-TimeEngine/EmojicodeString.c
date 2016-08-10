//
//  String.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeString.h"

#include <string.h>
#include <math.h>
#include <ctype.h>
#include "utf8.h"
#include "EmojicodeList.h"
#include "algorithms.h"

EmojicodeInteger stringCompare(String *a, String *b) {
    if (a == b) {
        return 0;
    }
    if (a->length != b->length) {
        return a->length - b->length;
    }
    
    return memcmp(a->characters->value, b->characters->value, a->length * sizeof(EmojicodeChar));
}

bool stringEqual(String *a, String *b){
    return stringCompare(a, b) == 0;
}

bool stringBeginsWith(String *a, String *with){
    if(a->length < with->length){
        return false;
    }
    
    return memcmp(a->characters->value, with->characters->value, with->length * sizeof(EmojicodeChar)) == 0;
}

bool stringEndsWith(String *a, String *end){
    if(a->length < end->length){
        return false;
    }
    
    return memcmp(((EmojicodeChar*)a->characters->value) + (a->length - end->length), end->characters->value, end->length * sizeof(EmojicodeChar)) == 0;
}

/** @warning GC-invoking */
Object* stringSubstring(Object *stro, EmojicodeInteger from, EmojicodeInteger length, Thread *thread){
    stackPush(somethingObject(stro), 1, 0, thread);
    {
        String *string = stackGetThisObject(thread)->value;
        if (from >= string->length){
            length = 0;
            from = 0;
        }
        
        if (length + from >= string->length){
            length = string->length - from;
        }
        
        if(length == 0){
            stackPop(thread);
            return emptyString;
        }
    }
    
    stackSetVariable(0, somethingObject(newObject(CL_STRING)), thread);
    Object *co = newArray(length * sizeof(EmojicodeChar));
    
    Object *ostro = stackGetVariable(0, thread).object;
    String *ostr = ostro->value;
    
    ostr->length = length;
    ostr->characters = co;
    
    memcpy(ostr->characters->value, characters((String *)stackGetThisObject(thread)->value) + from, length * sizeof(EmojicodeChar));
    
    stackPop(thread);
    return ostro;
}

void initStringFromSymbolList(Object *string, List *list){
    String *str = string->value;
    
    size_t count = list->count;
    str->length = count;
    str->characters = newArray(count * sizeof(EmojicodeChar));
    
    for (size_t i = 0; i < count; i++) {
        characters(str)[i] = (EmojicodeChar)listGet(list, i).raw;
    }
}

//MARK: Converting from C strings

char* stringToChar(String *str){
    //Size needed for UTF8 representation
    size_t ds = u8_codingsize(characters(str), str->length);
    //Allocate space for the UTF8 string
    char *utf8str = malloc(ds + 1);
    //Convert
    size_t written = u8_toutf8(utf8str, ds, characters(str), str->length);
    utf8str[written] = 0;
    return utf8str;
}

Object* stringFromChar(const char *cstring){
    EmojicodeInteger len = u8_strlen(cstring);
    
    if(len == 0){
        return emptyString;
    }
    
    Object *stro = newObject(CL_STRING);
    String *string = stro->value;
    string->length = len;
    string->characters = newArray(len * sizeof(EmojicodeChar));
    
    u8_toucs(characters(string), len, cstring, strlen(cstring));
    
    return stro;
}

//MARK: Bridges

static Something stringPrintStdoutBrigde(Thread *thread){
    String *string = stackGetThisObject(thread)->value;
    char *utf8str = stringToChar(string);
    printf("%s\n", utf8str);
    free(utf8str);
    return NOTHINGNESS;
}

static Something stringEqualBridge(Thread *thread){
    String *a = stackGetThisObject(thread)->value;
    String *b = stackGetVariable(0, thread).object->value;
    return stringEqual(a, b) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something stringSubstringBridge(Thread *thread){
    EmojicodeInteger from = unwrapInteger(stackGetVariable(0, thread));
    EmojicodeInteger length = unwrapInteger(stackGetVariable(1, thread));
    String *string = stackGetThisObject(thread)->value;
    
    if (from < 0) {
        from = (EmojicodeInteger)string->length + from;
    }
    if (length < 0) {
        length = (EmojicodeInteger)string->length + length;
    }
    
    return somethingObject(stringSubstring(stackGetThisObject(thread), from, length, thread));
}

static Something stringIndexOf(Thread *thread){
    String *string = stackGetThisObject(thread)->value;
    String *search = stackGetVariable(0, thread).object->value;
    
    void *location = findBytesInBytes(characters(string), string->length * sizeof(EmojicodeChar),
                                      characters(search), search->length * sizeof(EmojicodeChar));
    if (!location) {
        return NOTHINGNESS;
    }
    else {
        return somethingInteger(((Byte *)location - (Byte *)characters(string)) / sizeof(EmojicodeChar));
    }
}

static Something stringTrimBridge(Thread *thread){
    String *string = stackGetThisObject(thread)->value;
    
    EmojicodeInteger start = 0;
    EmojicodeInteger stop = string->length - 1;
    
    while(start < string->length && isWhitespace(characters(string)[start]))
        start++;
    
    while(stop > 0 && isWhitespace(characters(string)[stop]))
        stop--;
    
    return somethingObject(stringSubstring(stackGetThisObject(thread), start, stop - start + 1, thread));
}

static void stringGetInput(Thread *thread) {
    String *prompt = stackGetVariable(0, thread).object->value;
    char *utf8str = stringToChar(prompt);
    printf("%s\n", utf8str);
    fflush(stdout);
    free(utf8str);
    
    int bufferSize = 50, oldBufferSize = 0;
    Object *buffer = newArray(bufferSize);
    size_t bufferUsedSize = 0;
    
    while (true) {
        fgets((char *)buffer->value + oldBufferSize, bufferSize - oldBufferSize, stdin);
        
        bufferUsedSize = strlen(buffer->value);
        
        if(bufferUsedSize < bufferSize - 1){
            if (((char *)buffer->value)[bufferUsedSize - 1] == '\n') {
                bufferUsedSize -= 1;
            }
            break;
        }
        
        oldBufferSize = bufferSize - 1;
        bufferSize *= 2;
        buffer = resizeArray(buffer, bufferSize);
    }

    EmojicodeInteger len = u8_strlen_l(buffer->value, bufferUsedSize);
    
    String *string = stackGetThisObject(thread)->value;
    string->length = len;
    
    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = stackGetThisObject(thread)->value;
    string->characters = chars;
    
    u8_toucs(characters(string), len, buffer->value, bufferUsedSize);
}

static Something stringSplitByStringBridge(Thread *thread) {
    Something sp = stackGetVariable(0, thread);
    
    stackPush(stackGetThisContext(thread), 2, 0, thread);
    stackSetVariable(1, somethingObject(newObject(CL_LIST)), thread);
    stackSetVariable(0, sp, thread);
    
    EmojicodeInteger firstOfSeperator = 0, seperatorIndex = 0, firstAfterSeperator = 0;
    
    for (EmojicodeInteger i = 0, l = ((String *)stackGetThisObject(thread)->value)->length; i < l; i++) {
        Object *stringObject = stackGetThisObject(thread);
        Object *separatorObject = stackGetVariable(0, thread).object;
        String *separator = (String *)separatorObject->value;
        if(characters((String *)stringObject->value)[i] == characters(separator)[seperatorIndex]){
            if (seperatorIndex == 0) {
                firstOfSeperator = i;
            }
            
            //Full seperator found
            if(firstOfSeperator + separator->length == i + 1){
                Object *stro;
                if(firstAfterSeperator == firstOfSeperator){
                    stro = emptyString;
                }
                else {
                    stro = stringSubstring(stringObject, firstAfterSeperator, i - firstAfterSeperator - separator->length + 1, thread);
                }
                listAppend(stackGetVariable(1, thread).object, somethingObject(stro), thread);
                seperatorIndex = 0;
                firstAfterSeperator = i + 1;
            }
            else {
                seperatorIndex++;
            }
        }
        else if(seperatorIndex > 0){ //and not matching
            seperatorIndex = 0;
            //We search from the begin of the last partial match
            i = firstOfSeperator;
        }
    }
    
    Object *stringObject = stackGetThisObject(thread);
    String *string = (String *)stringObject->value;
    listAppend(stackGetVariable(1, thread).object, somethingObject(stringSubstring(stringObject, firstAfterSeperator, string->length - firstAfterSeperator, thread)), thread);
    
    Something list = stackGetVariable(1, thread);
    stackPop(thread);
    return list;
}

static Something stringLengthBridge(Thread *thread){
    String *string = stackGetThisObject(thread)->value;
    return somethingInteger((EmojicodeInteger)string->length);
}

static Something stringUTF8LengthBridge(Thread *thread){
    String *str = stackGetThisObject(thread)->value;
    return somethingInteger((EmojicodeInteger)u8_codingsize(str->characters->value, str->length));
}

static Something stringByAppendingSymbolBridge(Thread *thread){ //TODO: GC-Safety
    Object *co = newArray((((String *)stackGetThisObject(thread)->value)->length + 1) * sizeof(EmojicodeChar));
    String *string = stackGetThisObject(thread)->value;
    
    Object *ostro = newObject(CL_STRING);
    String *ostr = ostro->value;
    
    ostr->length = string->length + 1;
    ostr->characters = co;
    
    memcpy(characters(ostr), characters(string), string->length * sizeof(EmojicodeChar));
    
    characters(ostr)[string->length] = unwrapSymbol(stackGetVariable(0, thread));
    
    return somethingObject(ostro);
}

static Something stringSymbolAtBridge(Thread *thread){
    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    String *str = stackGetThisObject(thread)->value;
    if(index >= str->length){
        return NOTHINGNESS;
    }
    
    return somethingInteger(characters(str)[index]);
}

static Something stringBeginsWithBridge(Thread *thread){
    return stringBeginsWith(stackGetThisObject(thread)->value, stackGetVariable(0, thread).object->value) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something stringEndsWithBridge(Thread *thread){
    return stringEndsWith(stackGetThisObject(thread)->value, stackGetVariable(0, thread).object->value) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something stringSplitBySymbolBridge(Thread *thread){
    EmojicodeChar separator = unwrapSymbol(stackGetVariable(0, thread));
    stackPush(stackGetThisContext(thread), 1, 0, thread);
    stackSetVariable(0, somethingObject(newObject(CL_LIST)), thread);
    
    EmojicodeInteger from = 0;
    
    for (EmojicodeInteger i = 0, l = ((String *)stackGetThisObject(thread)->value)->length; i < l; i++) {
        Object *stringObject = stackGetThisObject(thread);
        if (characters((String *)stringObject->value)[i] == separator) {
            listAppend(stackGetVariable(0, thread).object, somethingObject(stringSubstring(stringObject, from, i - from, thread)), thread);
            from = i + 1;
        }
        
    }

    Object *stringObject = stackGetThisObject(thread);
    listAppend(stackGetVariable(0, thread).object, somethingObject(stringSubstring(stringObject, from, ((String *) stringObject->value)->length - from, thread)), thread);
    
    Something list = stackGetVariable(0, thread);
    stackPop(thread);
    return list;
}

static Something stringToData(Thread *thread){
    String *str = stackGetThisObject(thread)->value;
    
    size_t ds = u8_codingsize(characters(str), str->length);
    
    Object *bytesObject = newArray(ds);
    
    str = stackGetThisObject(thread)->value;
    u8_toutf8(bytesObject->value, ds, characters(str), str->length);
    
    stackPush(somethingObject(bytesObject), 0, 0, thread);
    
    Object *o = newObject(CL_DATA);
    Data *d = o->value;
    d->length = ds;
    d->bytesObject = stackGetThisObject(thread);
    d->bytes = d->bytesObject->value;
    
    stackPop(thread);
    
    return somethingObject(o);
}

static Something stringToCharacterList(Thread *thread){
    String *str = stackGetThisObject(thread)->value;
    Object *list = newObject(CL_LIST);
    
    for (size_t i = 0; i < str->length; i++) {
        listAppend(list, somethingSymbol(characters(str)[i]), thread);
    }
    return somethingObject(list);
}

static Something stringJSON(Thread *thread){
    return parseJSON(thread);
}

static void stringFromSymbolListBridge(Thread *thread){
    initStringFromSymbolList(stackGetThisObject(thread), stackGetVariable(0, thread).object->value);
}

static void stringFromStringList(Thread *thread) {
    size_t stringSize = 0;
    size_t appendLocation = 0;
    
    {
        List *list = stackGetVariable(0, thread).object->value;
        String *glue = stackGetVariable(1, thread).object->value;
        
        for (size_t i = 0; i < list->count; i++) {
            stringSize += ((String *)listGet(list, i).object->value)->length;
        }
        
        if (list->count > 0){
            stringSize += glue->length * (list->count - 1);
        }
    }
        
    Object *co = newArray(stringSize * sizeof(EmojicodeChar));
    
    {
        List *list = stackGetVariable(0, thread).object->value;
        String *glue = stackGetVariable(1, thread).object->value;
        
        String *string = stackGetThisObject(thread)->value;
        string->length = stringSize;
        string->characters = co;
        
        for (size_t i = 0; i < list->count; i++) {
            String *aString = listGet(list, i).object->value;
            memcpy(characters(string) + appendLocation, characters(aString), aString->length * sizeof(EmojicodeChar));
            appendLocation += aString->length;
            if(i + 1 < list->count){
                memcpy(characters(string) + appendLocation, characters(glue), glue->length * sizeof(EmojicodeChar));
                appendLocation += glue->length;
            }
        }
    }
}

static Something charactersToInteger(EmojicodeChar *characters, EmojicodeInteger base, EmojicodeInteger length) {
    if (length == 0) {
        return NOTHINGNESS;
    }
    EmojicodeInteger x = 0;
    for (size_t i = 0; i < length; i++) {
        if (i == 0 && (characters[i] == '-' || characters[i] == '+')) {
            if (length < 2) {
                return NOTHINGNESS;
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
            return NOTHINGNESS;
        }
        
        x *= base;
        x += b;
    }
    
    if (characters[0] == '-') {
        x *= -1;
    }
    return somethingInteger(x);
}

static Something stringToInteger(Thread *thread) {
    EmojicodeInteger base = stackGetVariable(0, thread).raw;
    String *string = (String *)stackGetThisObject(thread)->value;
    
    return charactersToInteger(characters(string), base, string->length);
}


static Something stringToDouble(Thread *thread){
    String *string = (String *)stackGetThisObject(thread)->value;
    
    if (string->length == 0) {
        return NOTHINGNESS;
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
                return NOTHINGNESS;
            } else {
                foundSeparator = true;
                continue;
            }
        }
        if (characters[i] == 'e' || characters[i] == 'E') {
            Something exponent = charactersToInteger(characters + i + 1, 10, string->length - i - 1);
            if (isNothingness(exponent)) {
                return NOTHINGNESS;
            } else {
                d *= pow(10, exponent.raw);
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
            return NOTHINGNESS;
        }
    }
    
    if (!foundDigit) {
        return NOTHINGNESS;
    }
    
    d /= pow(10, decimalPlace);
    
    if (!sign) {
        d *= -1;
    }
    return somethingDouble(d);
}

static Something stringToUppercase(Thread *thread) {
    Object *o = newObject(CL_STRING);
    size_t length = ((String *)stackGetThisObject(thread)->value)->length;
    stackPush(somethingObject(o), 0, 0, thread);
    Object *characters = newArray(length * sizeof(EmojicodeChar));
    o = stackGetThisObject(thread);
    String *news = o->value;
    news->characters = characters;
    news->length = length;
    stackPop(thread);
    String *os = stackGetThisObject(thread)->value;
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = characters(os)[i];
        if (c <= 'z') characters(news)[i] = toupper(c);
        else characters(news)[i] = c;
    }
    return somethingObject(o);
}

static Something stringToLowercase(Thread *thread) {
    Object *o = newObject(CL_STRING);
    size_t length = ((String *)stackGetThisObject(thread)->value)->length;
    stackPush(somethingObject(o), 0, 0, thread);
    Object *characters = newArray(length * sizeof(EmojicodeChar));
    o = stackGetThisObject(thread);
    String *news = o->value;
    news->characters = characters;
    news->length = length;
    stackPop(thread);
    String *os = stackGetThisObject(thread)->value;
    for (size_t i = 0; i < length; i++) {
        EmojicodeChar c = characters(os)[i];
        if (c <= 'z') characters(news)[i] = tolower(c);
        else characters(news)[i] = c;
    }
    return somethingObject(o);
}

static Something stringCompareBridge(Thread *thread) {
    String *a = stackGetThisObject(thread)->value;
    String *b = stackGetVariable(0, thread).object->value;
    return somethingInteger(stringCompare(a, b));
}

void stringMark(Object *self){
    if(((String *)self->value)->characters){
        mark(&((String *)self->value)->characters);
    }
}

FunctionFunctionPointer stringMethodForName(EmojicodeChar name){
    switch (name) {
        case 0x1F600:
            return stringPrintStdoutBrigde;
        case 0x1F61B:
            return stringEqualBridge;
        case 0x1f414: //🐔
            return stringLengthBridge;
        case 0x1f4dd: //📝
            return stringByAppendingSymbolBridge;
        case 0x1f43d: //🐽
            return stringSymbolAtBridge;
        case 0x1F52A:  //🔪
            return stringSubstringBridge;
        case 0x1F50D: //🔍
            return stringIndexOf;
        case 0x1F527: //🔧
            return stringTrimBridge;
        case 0x1F52B: //🔫
            return stringSplitByStringBridge;
        case 0x1F4D0: //📐
            return stringUTF8LengthBridge;
        case 0x1F4A3: //💣
            return stringSplitBySymbolBridge;
        case 0x1F3BC: //🎼
            return stringBeginsWithBridge;
        case 0x26F3: //⛳️
            return stringEndsWithBridge;
        case 0x1F3B6: //🎶
            return stringToCharacterList;
        case 0x1F4C7: //📇
            return stringToData;
        case 0x1f4f0: //📰
            return stringJSON;
        case 0x1F682: //🚂
            return stringToInteger;
        case 0x1F680: //🚀
            return stringToDouble;
        case 0x2194: //↔️
            return stringCompareBridge;
        case 0x1f4eb: //📫
            return stringToUppercase;
        case 0x1f4ea: //📪
            return stringToLowercase;
    }
    return NULL;
}

InitializerFunctionFunctionPointer stringInitializerForName(EmojicodeChar name){
    switch (name) {
        case 0x1F62F: //😮
            return stringGetInput;
        case 0x1F399: //🎙
            return stringFromSymbolListBridge;
        case 0x1F368: //🍨
            return stringFromStringList;
    }
    return NULL;
}
