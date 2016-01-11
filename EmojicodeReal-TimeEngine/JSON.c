//
//  JSON.c
//  Emojicode
//
//  Created by Theo Weidmann on 16/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "EmojicodeDictionary.h"
#include "EmojicodeList.h"
#include <math.h>

#define upgrade(now, expect, ec) case now: if (c == ec) { state = now ## expect; continue; } else jsonError();
#define upgradeReturn(now, ec, r) case now: if (c == ec) return r; else jsonError();
#define appendEscape(seq, c) case seq: state = JSON_STRING; listAppend(stackGetVariable(0, thread).object, somethingSymbol(c), thread); continue;
#define whitespaceCase case '\t': case '\n': case '\r': case ' ':
#define returnArray() Something s = stackGetVariable(0, thread);\
stackPop(thread);\
return s;

#define returnNumber() return (state == JSON_NUMBER_NEGATIVE) ? somethingInteger(-integer) : somethingInteger(integer);
#define returnDouble() return (state == JSON_DOUBLE_NEGATIVE) ? somethingDouble(-doubl) : somethingDouble(doubl);

typedef enum {
    JSON_NONE = 0,
    JSON_STRING = 1,
    JSON_STRING_ESCAPE = 2,
    JSON_NUMBER = 3, JSON_DOUBLE = 4, /* JSON_EXPONENT = 5, */
    JSON_NUMBER_NEGATIVE = 6, JSON_DOUBLE_NEGATIVE = 7, /* JSON_EXPONENT_NEGATIVE = 8, */
    JSON_OBJECT_KEY, JSON_OBJECT_COLON, JSON_OBJECT_VALUE, JSON_OBJECT_NEXT,
    JSON_ARRAY, JSON_ARRAY_NEXT, JSON_ARRAY_FIRST,
    JSON_TRUE_T, JSON_TRUE_TR, JSON_TRUE_TRU,
    JSON_FALSE_F, JSON_FALSE_FA, JSON_FALSE_FAL, JSON_FALSE_FALS,
    JSON_NULL_N, JSON_NULL_NU, JSON_NULL_NUL
} JSONState;

_Noreturn void error(char *err, ...);

_Noreturn void jsonError() {
    error("Error parsing json");
}

static Something JSONGetValue(const size_t length, size_t *i, Thread *thread){
    JSONState state = JSON_NONE;
    EmojicodeChar c;
    double doubl = 0;
    EmojicodeInteger integer = 0;
    int numberDigits = 0;
    
    while (*i < length) {
        c = characters((String *)stackGetThis(thread)->value)[(*i)++];
        
        switch (state) {
            case JSON_STRING:
                switch (c) {
                    case '\\':
                        state = JSON_STRING_ESCAPE;
                        continue;
                    case '"': {
                        stackSetVariable(1, somethingObject(newObject(CL_STRING)), thread);
                        initStringFromSymbolList(stackGetVariable(1, thread).object, stackGetVariable(0, thread).object->value);
                        Something s = stackGetVariable(1, thread);
                        stackPop(thread);
                        return s;
                    }
                    default:
                        listAppend(stackGetVariable(0, thread).object, somethingSymbol(c), thread);
                        continue;
                }
            case JSON_STRING_ESCAPE:
                switch (c) {
                    appendEscape('"', '"')
                    appendEscape('\\', '\\')
                    appendEscape('/', '/')
                    appendEscape('b', '\b')
                    appendEscape('f', '\f')
                    appendEscape('n', '\n')
                    appendEscape('r', '\r')
                    appendEscape('t', '\t')
                    //TODO: Unicode and hex
                }
                jsonError();
            case JSON_NUMBER:
            case JSON_NUMBER_NEGATIVE: {
                int a = c - 48;
                switch (c) {
                    case '.':
                        state += 1;
                        doubl = integer;
                        numberDigits = -1;
                        continue;
                    case 'E':
                    case 'e':
                        state += 2;
                        continue;
                    default:
                        if (a > 9 || a < 0) {
                            (*i)--;
                            returnNumber();
                        }
                }

                integer = integer * 10 + a;
                continue;
            }
            case JSON_DOUBLE:
            case JSON_DOUBLE_NEGATIVE: {
                double a = c - 48;
                switch (c) {
                    case 'E':
                    case 'e':
                        state += 1;
                        continue;
                    default:
                        if (a > 9 || a < 0) {
                            (*i)--;
                            returnDouble();
                        }
                }
                doubl += a * pow(10, numberDigits--);
                continue;
            }
            //TOOD: exponent
            case JSON_ARRAY_FIRST:
                state = JSON_ARRAY;
                if (c == ']') {
                    returnArray();
                }
            case JSON_ARRAY: {
                (*i)--;
                Something s = JSONGetValue(length, i, thread);
                listAppend(stackGetVariable(0, thread).object, s, thread);
                state = JSON_ARRAY_NEXT;
                continue;
            }
            case JSON_ARRAY_NEXT:
                switch (c) {
                    case ',':
                        state = JSON_ARRAY;
                        continue;
                    case ']': {
                        returnArray();
                    }
                    whitespaceCase
                        continue;
                    default:
                        jsonError();
                }
            case JSON_OBJECT_KEY: {
                (*i)--;
                Something s = JSONGetValue(length, i, thread);
                if (s.type != T_OBJECT || s.object->class != CL_STRING) {
                    jsonError();
                }
                stackSetVariable(1, s, thread);
                state = JSON_OBJECT_COLON;
                continue;
            }
            case JSON_OBJECT_COLON:
                switch (c) {
                    case ':':
                        state = JSON_OBJECT_VALUE;
                        continue;
                    whitespaceCase
                        continue;
                    default:
                        jsonError();
                }
            case JSON_OBJECT_VALUE: {
                (*i)--;
                Something s = JSONGetValue(length, i, thread);
                dictionarySet(stackGetVariable(0, thread).object, stackGetVariable(1, thread).object, s, thread);
                state = JSON_OBJECT_NEXT;
                continue;
            }
            case JSON_OBJECT_NEXT:
                switch (c) {
                    case ',':
                        state = JSON_OBJECT_KEY;
                        continue;
                    case '}': {
                        Something s = stackGetVariable(0, thread);
                        stackPop(thread);
                        return s;
                    }
                    whitespaceCase
                        continue;
                    default:
                        jsonError();
                }
            upgrade(JSON_TRUE_T, R, 'r')
            upgrade(JSON_TRUE_TR, U, 'u')
            upgradeReturn(JSON_TRUE_TRU, 'e', EMOJICODE_TRUE)
            upgrade(JSON_FALSE_F, A, 'a')
            upgrade(JSON_FALSE_FA, L, 'l')
            upgrade(JSON_FALSE_FAL, S, 's')
            upgradeReturn(JSON_FALSE_FALS, 'e', EMOJICODE_FALSE)
            upgrade(JSON_NULL_N, U, 'u')
            upgrade(JSON_NULL_NU, L, 'l')
            upgradeReturn(JSON_NULL_NUL, 'l', NOTHINGNESS)
            case JSON_NONE:
                switch (c) {
                    case '"':
                        state = JSON_STRING;
                        stackPush(stackGetThis(thread), 2, 0, thread);
                        Object *string = newObject(CL_LIST);
                        stackSetVariable(0, somethingObject(string), thread);
                        break;
                    case '{':
                        state = JSON_OBJECT_KEY;
                        stackPush(newObject(CL_DICTIONARY), 0, 0, thread);
                        dictionaryInit(thread);
                        Object *o = stackGetThis(thread);
                        stackPop(thread);
                        stackPush(stackGetThis(thread), 2, 0, thread);
                        stackSetVariable(0, somethingObject(o), thread);
                        break;
                    case '[':
                        state = JSON_ARRAY_FIRST;
                        stackPush(stackGetThis(thread), 1, 0, thread);
                        stackSetVariable(0, somethingObject(newObject(CL_LIST)), thread);
                        break;
                    case 't':
                        state = JSON_TRUE_T;
                        break;
                    case 'f':
                        state = JSON_FALSE_F;
                        break;
                    case 'n':
                        state = JSON_NULL_N;
                        break;
                    case '-':
                        state = JSON_NUMBER_NEGATIVE;
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        state = JSON_NUMBER;
                        integer = c - 48;
                        break;
                    whitespaceCase
                        continue;
                    default:
                        jsonError();
                }
        }
    }
    switch (state) {
        case JSON_DOUBLE:
        case JSON_DOUBLE_NEGATIVE:
            returnDouble();
        case JSON_NUMBER:
        case JSON_NUMBER_NEGATIVE:
            returnNumber();
        default:
            jsonError();
    }
}

Something parseJSON(Thread *thread){
    size_t i = 0;
    return JSONGetValue(((String*)stackGetThis(thread)->value)->length, &i, thread);
}