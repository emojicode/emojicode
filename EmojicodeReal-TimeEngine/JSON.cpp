//
//  JSON.c
//  Emojicode
//
//  Created by Theo Weidmann on 16/12/15.
//  Copyright © 2015 Theo Weidmann. All rights reserved.
//

#include "Engine.hpp"
#include "Dictionary.hpp"
#include "List.hpp"
#include "Thread.hpp"
#include <cmath>

namespace Emojicode {

enum JSONState {
    JSON_NONE = 0,
    JSON_STRING = 1,
    JSON_STRING_ESCAPE = 2,
    JSON_NUMBER = 3, JSON_DOUBLE = 4, JSON_EXPONENT = 5, JSON_EXPONENT_NO_PLUS = 6, JSON_EXPONENT_BACK_VALUE = 7,
    JSON_NUMBER_NEGATIVE = 8, JSON_DOUBLE_NEGATIVE = 9, JSON_EXPONENT_NEGATIVE = 10,
    JSON_EXPONENT_NO_PLUS_NEGATIVE = 11, JSON_EXPONENT_BACK_VALUE_NEGATIVE = 12,
    JSON_OBJECT_KEY, JSON_OBJECT_KEY_BACK_VALUE, JSON_OBJECT_VALUE_BACK_VALUE,
    JSON_ARRAY, JSON_ARRAY_BACK_VALUE, JSON_ARRAY_FIRST,
    JSON_TRUE_T, JSON_TRUE_TR, JSON_TRUE_TRU,
    JSON_FALSE_F, JSON_FALSE_FA, JSON_FALSE_FAL, JSON_FALSE_FALS,
    JSON_NULL_N, JSON_NULL_NU, JSON_NULL_NUL
};

struct JSONStackFrame {
    int state;

    EmojicodeInteger integer;
    int numberDigitsFraction;
    RetainedObjectPointer object = RetainedObjectPointer(nullptr);
    RetainedObjectPointer secondaryObject = RetainedObjectPointer(nullptr);
};

#define errorExit() thread->returnNothingnessFromFunction(); return;
#define upgrade(now, expect, ec) case now: if (c == (ec)) { stackCurrent->state = now ## expect; continue; } else { errorExit(); }
#define upgradeReturn(now, ec, r) case now: if (c == (ec)) { backValue = r; popTheStack(); } else { errorExit(); }
#define appendEscape(seq, c) case seq: *listAppendDestination(stackCurrent->object, thread) = Box(T_SYMBOL, EmojicodeChar(c)); continue;
#define whitespaceCase case '\t': case '\n': case '\r': case ' ':
#define popTheStack() if (stackCurrent->secondaryObject.unretainedPointer()) thread->release(1); if (stackCurrent->object.unretainedPointer()) thread->release(1); stackCurrent--; continue;
#define pushTheStack() stackCurrent++; if (stackCurrent > stackLimit) { errorExit(); } stackCurrent->state = JSON_NONE; stackCurrent->secondaryObject = RetainedObjectPointer(nullptr); stackCurrent->object = RetainedObjectPointer(nullptr); continue;
#define returnArray() backValue = Box(T_OBJECT, stackCurrent->object.unretainedPointer()); popTheStack();
#define integerValue() Box(T_INTEGER, (stackCurrent->state == JSON_NUMBER_NEGATIVE) ? -stackCurrent->integer : stackCurrent->integer)

#define _doubleInternalValue() ((double)stackCurrent->integer/pow(10, stackCurrent->numberDigitsFraction))
#define doubleRawValue() ((stackCurrent->state == JSON_DOUBLE_NEGATIVE) ? -_doubleInternalValue() : _doubleInternalValue())
#define doubleValue() Box(T_DOUBLE, doubleRawValue())

#define jsonMaxDepth 256

void stringJSON(Thread *thread) {
    const size_t length = thread->thisObject()->val<String>()->length;
    JSONStackFrame stack[jsonMaxDepth];
    JSONStackFrame *stackLimit = stack + jsonMaxDepth - 1;
    JSONStackFrame *stackCurrent = stack;
    EmojicodeChar c;
    Box backValue;
    size_t i = 0;

    stackCurrent->state = JSON_NONE;
    stackCurrent->object = RetainedObjectPointer(nullptr);
    stackCurrent->secondaryObject = RetainedObjectPointer(nullptr);

    while (i < length) {
        if (stackCurrent < stack) {
            errorExit();
        }

        c = thread->thisObject()->val<String>()->characters()[i++];

        switch (stackCurrent->state) {
            case JSON_STRING:
                switch (c) {
                    case '\\':
                        stackCurrent->state = JSON_STRING_ESCAPE;
                        continue;
                    case '"': {
                        auto stringObject = thread->retain(newObject(CL_STRING));
                        initStringFromSymbolList(stringObject->val<String>(), stackCurrent->object->val<List>());
                        thread->release(1);
                        backValue = Box(T_OBJECT, stringObject.unretainedPointer());
                        popTheStack();
                    }
                    default:
                        if (c <= 0x1F) {
                            errorExit();
                        }
                        *listAppendDestination(stackCurrent->object, thread) = Box(T_SYMBOL, c);
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
                        EmojicodeChar *chars = thread->thisObject()->val<String>()->characters();
                        EmojicodeInteger x = 0, high = 0;
                        while (true) {
                            for (size_t e = i + 4; i < e; i++) {
                                if (i >= length) {
                                    errorExit();
                                }

                                c = chars[i];
                                x *= 16;

                                if ('0' <= c && c <= '9') {
                                    x += c - '0';
                                }
                                else if ('A' <= c && c <= 'F') {
                                    x += c - 'A' + 10;
                                }
                                else if ('a' <= c && c <= 'f') {
                                    x += c - 'a' + 10;
                                }
                                else {
                                    errorExit();
                                }
                            }
                            if (high > 0) {
                                x = (high << 10) + x + 0x10000 - (0xD800 << 10) - 0xDC00;
                            }
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
                        *listAppendDestination(stackCurrent->object, thread) = Box(T_SYMBOL, x);
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
                if (c == '+') {
                    continue;
                }
            case JSON_EXPONENT_NO_PLUS:
            case JSON_EXPONENT_NO_PLUS_NEGATIVE:
                stackCurrent->state++;
                pushTheStack();
            case JSON_EXPONENT_BACK_VALUE:
            case JSON_EXPONENT_BACK_VALUE_NEGATIVE: {
                if (backValue.type.raw != T_INTEGER) {
                    errorExit();
                }
                backValue = Box(T_DOUBLE, doubleRawValue() * pow(10, backValue.value1.raw));
            }
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
                *listAppendDestination(stackCurrent->object, thread) = backValue;

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
                if (backValue.type.raw != T_OBJECT || backValue.value1.object->klass != CL_STRING) {
                    errorExit();
                }
                stackCurrent->secondaryObject = thread->retain(backValue.value1.object);

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
                *dictionaryPutVal(stackCurrent->object, stackCurrent->secondaryObject, thread) = backValue;

                switch (c) {
                    case ',':
                        stackCurrent->state = JSON_OBJECT_KEY;
                        continue;
                    case '}':
                        backValue = Box(T_OBJECT, stackCurrent->object.unretainedPointer());
                        popTheStack();
                        whitespaceCase
                        continue;
                    default:
                        errorExit();
                }
                upgrade(JSON_TRUE_T, R, 'r')
                upgrade(JSON_TRUE_TR, U, 'u')
                upgradeReturn(JSON_TRUE_TRU, 'e', Box(T_BOOLEAN, EmojicodeInteger(1)))
                upgrade(JSON_FALSE_F, A, 'a')
                upgrade(JSON_FALSE_FA, L, 'l')
                upgrade(JSON_FALSE_FAL, S, 's')
                upgradeReturn(JSON_FALSE_FALS, 'e', Box(T_BOOLEAN, EmojicodeInteger(0)))
                upgrade(JSON_NULL_N, U, 'u')
                upgrade(JSON_NULL_NU, L, 'l')
                upgradeReturn(JSON_NULL_NUL, 'l', Box(T_NOTHINGNESS, EmojicodeInteger(0)))
            case JSON_NONE:
                switch (c) {
                    case '"': {
                        stackCurrent->state = JSON_STRING;
                        stackCurrent->object = thread->retain(newObject(CL_LIST));
                        break;
                    }
                    case '{': {
                        stackCurrent->state = JSON_OBJECT_KEY;
                        stackCurrent->object = thread->retain(newObject(CL_DICTIONARY));
                        dictionaryInit(stackCurrent->object->val<EmojicodeDictionary>());
                        break;
                    }
                    case '[': {
                        stackCurrent->state = JSON_ARRAY_FIRST;
                        stackCurrent->object = thread->retain(newObject(CL_LIST));
                        break;
                    }
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
                thread->returnFromFunction(doubleValue());
                return;
            case JSON_NUMBER:
            case JSON_NUMBER_NEGATIVE:
                thread->returnFromFunction(integerValue());
                return;
            default:
                break;
        }
    }

    if (stackCurrent < stack) {
        thread->returnFromFunction(backValue);
        return;
    }
    
    errorExit();
}

}  // namespace Emojicode
