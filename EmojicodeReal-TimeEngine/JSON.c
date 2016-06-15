//
//  JSON.c
//  Emojicode
//
//  Created by Theo Weidmann on 16/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#include "Emojicode.h"
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
    EmojicodeInteger integer;
    int numberDigitsFraction;
} JSONStackFrame;

#define errorExit() restoreStackState(s, thread); return NOTHINGNESS;
#define upgrade(now, expect, ec) case now: if (c == ec) { stackCurrent->state = now ## expect; continue; } else { errorExit(); }
#define upgradeReturn(now, ec, r) case now: if (c == ec) { backValue = r; popTheStack(); } else { errorExit(); }
#define appendEscape(seq, c) case seq: listAppend(stackGetVariable(0, thread).object, somethingSymbol(c), thread); continue;
#define whitespaceCase case '\t': case '\n': case '\r': case ' ':
#define popTheStack() stackCurrent--; continue;
#define pushTheStack() stackCurrent++; if (stackCurrent > stackLimit) { errorExit(); } stackCurrent->state = JSON_NONE; continue;
#define returnArray() backValue = stackGetVariable(0, thread); stackPop(thread); popTheStack();
#define integerValue() somethingInteger((stackCurrent->state == JSON_NUMBER_NEGATIVE) ? -stackCurrent->integer : stackCurrent->integer)

#define _doubleInternalValue() ((double)stackCurrent->integer/pow(10, stackCurrent->numberDigitsFraction))
#define doubleRawValue() ((stackCurrent->state == JSON_DOUBLE_NEGATIVE) ? -_doubleInternalValue() : _doubleInternalValue())
#define doubleValue() somethingDouble(doubleRawValue())

#define jsonMaxDepth 256

Something parseJSON(Thread *thread) {
    const size_t length = ((String*)stackGetThisObject(thread)->value)->length;
    JSONStackFrame stack[jsonMaxDepth];
    JSONStackFrame *stackLimit = stack + jsonMaxDepth - 1;
    JSONStackFrame *stackCurrent = stack;
    EmojicodeChar c;
    Something backValue = NOTHINGNESS;
    size_t i = 0;
    
    StackState s = storeStackState(thread);
    
    stackCurrent->state = JSON_NONE;
    
    while (i < length) {
        if (stackCurrent < stack) {
            errorExit();
        }
        
        c = characters((String *)stackGetThisObject(thread)->value)[i++];
        
        switch (stackCurrent->state) {
            case JSON_STRING:
                switch (c) {
                    case '\\':
                        stackCurrent->state = JSON_STRING_ESCAPE;
                        continue;
                    case '"':
                        stackSetVariable(1, somethingObject(newObject(CL_STRING)), thread);
                        initStringFromSymbolList(stackGetVariable(1, thread).object, stackGetVariable(0, thread).object->value);
                        backValue = stackGetVariable(1, thread);
                        stackPop(thread);
                        popTheStack();
                    default:
                        if (c <= 0x1F) {
                            errorExit();
                        }
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
                    case 'u': {
                        EmojicodeChar *chars = characters((String *)stackGetThisObject(thread)->value);
                        EmojicodeInteger x = 0, high = 0;
                        while (true) {
                            for (size_t e = i + 4; i < e; i++) {
                                if (i >= length) {
                                    errorExit();
                                }
                                
                                c = chars[i];
                                x *= 16;
                                
                                if ('0' <= c && c <= '9')
                                    x += c - '0';
                                else if ('A' <= c && c <= 'F')
                                    x += c - 'A' + 10;
                                else if ('a' <= c && c <= 'f')
                                    x += c - 'a' + 10;
                                else {
                                    errorExit();
                                }
                            }
                            if (high)
                                x = (high << 10) + x + 0x10000 - (0xD800 << 10) - 0xDC00;
                            else if (0xD800 <= x && x <= 0xDBFF) {
                                if (i + 2 >= length || chars[i++] != '\\' || chars[i++] != 'u') {
                                    errorExit();
                                }
                                high = x;
                                x = 0;
                                continue;
                            }
                            break;
                        }
                        listAppend(stackGetVariable(0, thread).object, somethingSymbol(x), thread);
                        continue;
                    }
                }
                errorExit();
            case JSON_DOUBLE:
            case JSON_DOUBLE_NEGATIVE:
                stackCurrent->numberDigitsFraction++;
            case JSON_NUMBER:
            case JSON_NUMBER_NEGATIVE:
                switch (c) {
                    case '.':
                        stackCurrent->state += 1;
                        stackCurrent->numberDigitsFraction = 0;
                        continue;
                    case 'E':
                    case 'e':
                        stack->numberDigitsFraction = 0;
                        stackCurrent->state += 2;
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
                        stackCurrent->integer = stackCurrent->integer * 10 + c - '0';
                        continue;
                    default:
                        i--;
                        backValue = integerValue();
                        popTheStack();
                }
            case JSON_EXPONENT:
            case JSON_EXPONENT_NEGATIVE:
                stackCurrent->state++;
                if (c == '+') continue;
            case JSON_EXPONENT_NO_PLUS:
            case JSON_EXPONENT_NO_PLUS_NEGATIVE:
                stackCurrent->state++;
                pushTheStack();
            case JSON_EXPONENT_BACK_VALUE:
            case JSON_EXPONENT_BACK_VALUE_NEGATIVE:
                if (backValue.type != T_INTEGER) {
                    errorExit();
                }
                double x = doubleRawValue() * pow(10, backValue.raw);
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
                        errorExit();
                }
            case JSON_OBJECT_KEY:
                i--;
                stackCurrent->state = JSON_OBJECT_KEY_BACK_VALUE;
                pushTheStack();
            case JSON_OBJECT_KEY_BACK_VALUE:
                if (backValue.type != T_OBJECT || backValue.object->class != CL_STRING) {
                    errorExit();
                }
                stackSetVariable(1, backValue, thread);
                stackCurrent->state = JSON_OBJECT_COLON;
                i--;
                continue;
            case JSON_OBJECT_COLON:
                switch (c) {
                    case ':':
                        stackCurrent->state = JSON_OBJECT_VALUE_BACK_VALUE;
                        pushTheStack();
                    whitespaceCase
                        continue;
                    default:
                        errorExit();
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
                        errorExit();
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
                        stackPush(stackGetThisContext(thread), 2, 0, thread);
                        Object *string = newObject(CL_LIST);
                        stackSetVariable(0, somethingObject(string), thread);
                        break;
                    case '{':
                        stackCurrent->state = JSON_OBJECT_KEY;
                        stackPush(somethingObject(newObject(CL_DICTIONARY)), 0, 0, thread);
                        dictionaryInit(thread);
                        Object *o = stackGetThisObject(thread);
                        stackPop(thread);
                        stackPush(somethingObject(stackGetThisObject(thread)), 2, 0, thread);
                        stackSetVariable(0, somethingObject(o), thread);
                        break;
                    case '[':
                        stackCurrent->state = JSON_ARRAY_FIRST;
                        stackPush(somethingObject(stackGetThisObject(thread)), 1, 0, thread);
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
                        stackCurrent->integer = 0;
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
                        stackCurrent->integer = c - '0';
                        break;
                    whitespaceCase
                        continue;
                    default:
                        errorExit();
                }
        }
    }
    
    if (stackCurrent == stack) {
        switch (stackCurrent->state) {
            case JSON_DOUBLE:
            case JSON_DOUBLE_NEGATIVE:
                return doubleValue();
            case JSON_NUMBER:
            case JSON_NUMBER_NEGATIVE:
                return integerValue();
            default:
                break;
        }
    }
    
    if (stackCurrent < stack) {
        return backValue;
    }
    
    errorExit();
}