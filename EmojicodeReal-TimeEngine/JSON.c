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

typedef enum {
    JSON_NONE = 0,
    JSON_STRING = 1,
    JSON_STRING_ESCAPE = 2,
    JSON_NUMBER = 3, JSON_DOUBLE = 4, JSON_EXPONENT = 5, JSON_EXPONENT_NO_PLUS = 6, JSON_EXPONENT_BACK_VALUE = 7,
    JSON_NUMBER_NEGATIVE = 8, JSON_DOUBLE_NEGATIVE = 9, JSON_EXPONENT_NEGATIVE = 10, JSON_EXPONENT_NO_PLUS_NEGATIVE = 11, JSON_EXPONENT_BACK_VALUE_NEGATIVE = 12,
    JSON_OBJECT_KEY, JSON_OBJECT_KEY_BACK_VALUE, JSON_OBJECT_COLON, JSON_OBJECT_VALUE_BACK_VALUE, JSON_OBJECT_NEXT,
    JSON_ARRAY, JSON_ARRAY_BACK_VALUE, JSON_ARRAY_NEXT, JSON_ARRAY_FIRST,
    JSON_TRUE_T, JSON_TRUE_TR, JSON_TRUE_TRU,
    JSON_FALSE_F, JSON_FALSE_FA, JSON_FALSE_FAL, JSON_FALSE_FALS,
    JSON_NULL_N, JSON_NULL_NU, JSON_NULL_NUL
} JSONState;

typedef struct {
    JSONState state;
    union {
        double doubl;
        EmojicodeInteger integer;
    };
    int numberDigits;
} JSONStackFrame;

#define upgrade(now, expect, ec) case now: if (c == ec) { stackCurrent->state = now ## expect; continue; } else return NOTHINGNESS;
#define upgradeReturn(now, ec, r) case now: if (c == ec) { backValue = r; popTheStack(); } else return NOTHINGNESS;
#define appendEscape(seq, c) case seq: listAppend(stackGetVariable(0, thread).object, somethingSymbol(c), thread); continue;
#define whitespaceCase case '\t': case '\n': case '\r': case ' ':
#define popTheStack() stackCurrent--; continue;
#define pushTheStack() stackCurrent++; if (stackCurrent > stackLimit) return NOTHINGNESS; stackCurrent->state = JSON_NONE; continue;
#define returnArray() backValue = stackGetVariable(0, thread); stackPop(thread); popTheStack();
#define integerValue() somethingInteger((stackCurrent->state == JSON_NUMBER_NEGATIVE) ? -stackCurrent->integer : stackCurrent->integer)
#define doubleValue() somethingDouble((stackCurrent->state == JSON_DOUBLE_NEGATIVE) ? -stackCurrent->doubl : stackCurrent->doubl)

static Something JSONGetValue(const size_t length, Thread *thread){
    JSONStackFrame stack[256];
    JSONStackFrame *stackLimit = stack + 255;
    JSONStackFrame *stackCurrent = stack;
    EmojicodeChar c;
    Something backValue = NOTHINGNESS;
    size_t i = 0;
    
    stackCurrent->state = JSON_NONE;
    
    while (i < length) {
        if (stackCurrent < stack) {
            return NOTHINGNESS;
        }
        
        c = characters((String *)stackGetThis(thread)->value)[i++];
        
        switch (stackCurrent->state) {
            case JSON_STRING:
                switch (c) {
                    case '\\':
                        stackCurrent->state = JSON_STRING_ESCAPE;
                        continue;
                    case '"':
                        stackSetVariable(1, somethingObject(newObject(CL_STRING)), thread);
                        initStringFromSymbolList(stackGetVariable(1, thread).object, stackGetVariable(0, thread).object->value);
                        Something s = stackGetVariable(1, thread);
                        stackPop(thread);
                        backValue = s;
                        popTheStack();
                    default:
                        listAppend(stackGetVariable(0, thread).object, somethingSymbol(c), thread);
                        continue;
                }
            case JSON_STRING_ESCAPE:
                stackCurrent->state = JSON_STRING;
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
                return NOTHINGNESS;
            case JSON_NUMBER:
            case JSON_NUMBER_NEGATIVE: {
                int a = c - 48;
                switch (c) {
                    case '.': {
                        stackCurrent->state += 1;
                        EmojicodeInteger integer = stackCurrent->integer;
                        stackCurrent->doubl = integer;
                        stackCurrent->numberDigits = -1;
                        continue;
                    }
                    case 'E':
                    case 'e': {
                        EmojicodeInteger integer = stackCurrent->integer;
                        stackCurrent->doubl = integer;
                        stackCurrent->state += 2;
                        continue;
                    }
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
                        stack->integer = stack->integer * 10 + a;
                        continue;
                    default:
                        i--;
                        backValue = integerValue();
                        popTheStack();
                }
            }
            case JSON_DOUBLE:
            case JSON_DOUBLE_NEGATIVE: {
                double a = c - 48;
                switch (c) {
                    case 'E':
                    case 'e':
                        stackCurrent->state += 1;
                        continue;
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
                        stackCurrent->doubl += a * pow(10, stackCurrent->numberDigits--);
                        continue;
                    default:
                        i--;
                        backValue = doubleValue();
                        popTheStack();
                }
                
            }
            case JSON_EXPONENT:
            case JSON_EXPONENT_NEGATIVE:
                stackCurrent->state++;
                if (c == '+') continue;
                // Intentional fallthrough
            case JSON_EXPONENT_NO_PLUS:
            case JSON_EXPONENT_NO_PLUS_NEGATIVE:
                stackCurrent->state++;
                pushTheStack();
            case JSON_EXPONENT_BACK_VALUE:
            case JSON_EXPONENT_BACK_VALUE_NEGATIVE:
                if (backValue.type != T_INTEGER) {
                    return NOTHINGNESS;
                }
                double x = stackCurrent->doubl * pow(10, backValue.raw);
                backValue = somethingDouble(x);
            case JSON_ARRAY_FIRST:
                stackCurrent->state = JSON_ARRAY;
                if (c == ']') {
                    returnArray();
                }
            case JSON_ARRAY:
                i--;
                stackCurrent->state = JSON_ARRAY_BACK_VALUE;
                pushTheStack();
            case JSON_ARRAY_BACK_VALUE:
                listAppend(stackGetVariable(0, thread).object, backValue, thread);
                stackCurrent->state = JSON_ARRAY_NEXT;
                i--;
                continue;
            case JSON_ARRAY_NEXT:
                switch (c) {
                    case ',':
                        stackCurrent->state = JSON_ARRAY;
                        continue;
                    case ']':
                        returnArray();
                    whitespaceCase
                        continue;
                    default:
                        return NOTHINGNESS;
                }
            case JSON_OBJECT_KEY:
                i--;
                stackCurrent->state = JSON_OBJECT_KEY_BACK_VALUE;
                pushTheStack();
            case JSON_OBJECT_KEY_BACK_VALUE:
                if (backValue.type != T_OBJECT || backValue.object->class != CL_STRING) {
                    return NOTHINGNESS;
                }
                stackSetVariable(1, backValue, thread);
                stackCurrent->state = JSON_OBJECT_COLON;
                i--;
                continue;
            case JSON_OBJECT_COLON:
                switch (c) {
                    case ':':
                        stackCurrent->state = JSON_OBJECT_VALUE_BACK_VALUE;
                        pushTheStack()
                    whitespaceCase
                        continue;
                    default:
                        return NOTHINGNESS;
                }
            case JSON_OBJECT_VALUE_BACK_VALUE:
                dictionarySet(stackGetVariable(0, thread).object, stackGetVariable(1, thread).object, backValue, thread);
                stackCurrent->state = JSON_OBJECT_NEXT;
                i--;
                continue;
            case JSON_OBJECT_NEXT:
                switch (c) {
                    case ',':
                        stackCurrent->state = JSON_OBJECT_KEY;
                        continue;
                    case '}':
                        backValue = stackGetVariable(0, thread);
                        stackPop(thread);
                        popTheStack();
                    whitespaceCase
                        continue;
                    default:
                        return NOTHINGNESS;
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
                        stackCurrent->state = JSON_STRING;
                        stackPush(stackGetThis(thread), 2, 0, thread);
                        Object *string = newObject(CL_LIST);
                        stackSetVariable(0, somethingObject(string), thread);
                        break;
                    case '{':
                        stackCurrent->state = JSON_OBJECT_KEY;
                        stackPush(newObject(CL_DICTIONARY), 0, 0, thread);
                        dictionaryInit(thread);
                        Object *o = stackGetThis(thread);
                        stackPop(thread);
                        stackPush(stackGetThis(thread), 2, 0, thread);
                        stackSetVariable(0, somethingObject(o), thread);
                        break;
                    case '[':
                        stackCurrent->state = JSON_ARRAY_FIRST;
                        stackPush(stackGetThis(thread), 1, 0, thread);
                        stackSetVariable(0, somethingObject(newObject(CL_LIST)), thread);
                        break;
                    case 't':
                        stackCurrent->state = JSON_TRUE_T;
                        break;
                    case 'f':
                        stackCurrent->state = JSON_FALSE_F;
                        break;
                    case 'n':
                        stackCurrent->state = JSON_NULL_N;
                        break;
                    case '-':
                        stackCurrent->state = JSON_NUMBER_NEGATIVE;
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
                        stackCurrent->state = JSON_NUMBER;
                        stackCurrent->integer = c - 48;
                        break;
                    whitespaceCase
                        continue;
                    default:
                        return NOTHINGNESS;
                }
        }
    }
    switch (stackCurrent->state) {
        case JSON_DOUBLE:
        case JSON_DOUBLE_NEGATIVE:
            return doubleValue();
        case JSON_NUMBER:
        case JSON_NUMBER_NEGATIVE:
            return integerValue();
        default:
            return backValue;
    }
}

Something parseJSON(Thread *thread){
    return JSONGetValue(((String*)stackGetThis(thread)->value)->length, thread);
}