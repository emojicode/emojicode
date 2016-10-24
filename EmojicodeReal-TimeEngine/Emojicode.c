//
//  Emojicode.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <locale.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "Emojicode.h"

#include "EmojicodeList.h"
#include "EmojicodeString.h"
#include "EmojicodeDictionary.h"
#include "utf8.h"

Class *CL_OBJECT = NULL;
Class *CL_STRING;
Class *CL_LIST;
Class *CL_ERROR;
Class *CL_DATA;
Class *CL_DICTIONARY;
Class *CL_CAPTURED_FUNCTION_CALL;
Class *CL_CLOSURE;
Class *CL_RANGE;

static Class cl_array = {
    NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0, NULL, NULL, 0, 0
};
Class *CL_ARRAY = &cl_array;

char **cliArguments;
int cliArgumentCount;

const char *packageDirectory = defaultPackagesDirectory;

Class **classTable;
Function **functionTable;

uint_fast16_t stringPoolCount;
Object **stringPool;


//MARK: Coins

EmojicodeCoin consumeCoin(Thread *thread){
    return *(thread->tokenStream++);
}

static EmojicodeCoin nextCoin(Thread *thread){
    return *thread->tokenStream;
}

//MARK: Error

_Noreturn void error(char *err, ...){
    va_list list;
    va_start(list, err);
    
    char error[350];
    vsprintf(error, err, list);
    
    fprintf(stderr, "🚨 Fatal Error: %s\n", error);
    
    va_end(list);
    exit(1);
}

//MARK: Block utilities

static void passBlock(Thread *thread){
    EmojicodeCoin length = consumeCoin(thread);
    thread->tokenStream += length; //This coin only contains the length of the block
}

/** 
 * Runs a block. If a red apple appears or a red apple delegation is detected, appropriate steps are taken and
 * the method returns true. The calling function (should be @c parse) must immediately return.
 * Every other block ran by runBlock will also automatically respond.
 */
static bool runBlock(Thread *thread){
    EmojicodeCoin length = consumeCoin(thread); //This token only contains the length of the block
    
    EmojicodeCoin *end = thread->tokenStream + length;
    while (thread->tokenStream < end) {
        parse(consumeCoin(thread), thread);
        
        pauseForGC(NULL);
        
        if(thread->returned){
            return true;
        }
    }
    
    return false;
}

static Something runFunctionPointerBlock(Thread *thread, uint32_t length){
    EmojicodeCoin *end = thread->tokenStream + length;
    while (thread->tokenStream < end) {
        EmojicodeCoin c = consumeCoin(thread);

        parse(c, thread);
        
        pauseForGC(NULL);
        
        if(thread->returned){
            Something ret = thread->returnValue;
            thread->returned = false;
            return ret;
        }
    }
    return NOTHINGNESS;
}

static Class* readClass(Thread *thread) {
    return parse(consumeCoin(thread), thread).eclass;
}

static double readDouble(Thread *thread) {
    EmojicodeInteger scale = ((EmojicodeInteger)consumeCoin(thread) << 32) ^ consumeCoin(thread);
    EmojicodeInteger exp = consumeCoin(thread);
    
    return ldexp((double)scale/PORTABLE_INTLEAST64_MAX, (int)exp);
}


//MARK:

bool isNothingness(Something sth) {
    return sth.type == T_OBJECT && sth.object == NULL;
}

bool isRealObject(Something sth) {
    return sth.type == T_OBJECT && sth.object;
}

Something unwrapOptional(Something sth) {
    if (isNothingness(sth)) {
        error("Unexpectedly found ✨ while unwrapping a 🍬.");
    }
    return sth;
}

//MARK: Low level parsing

Something executeCallableExtern(Object *callable, Something *args, Thread *thread){
    if (callable->class == CL_CAPTURED_FUNCTION_CALL) {
        CapturedFunctionCall *cmc = callable->value;
        Function *method = cmc->function;
        
        Something ret;
        if (method->native) {
            Something *t = stackReserveFrame(cmc->callee, method->argumentCount, thread);
            memcpy(t, args, method->argumentCount * sizeof(Something));
            stackPushReservedFrame(thread);
            ret = method->handler(thread);
        }
        else {
            Something *t = stackReserveFrame(cmc->callee, method->variableCount, thread);
            memcpy(t, args, method->argumentCount * sizeof(Something));
            stackPushReservedFrame(thread);
            
            EmojicodeCoin *preCoinStream = thread->tokenStream;
            
            thread->tokenStream = method->tokenStream;
            
            ret = runFunctionPointerBlock(thread, method->tokenCount);
            
            thread->tokenStream = preCoinStream;
        }
        stackPop(thread);
        return ret;
    }
    else {
        Closure *c = callable->value;
        
        Something *t = stackReserveFrame(c->thisContext, c->variableCount, thread);
        memcpy(t, args, c->argumentCount * sizeof(Something));
        stackPushReservedFrame(thread);
        
        Something *cv = c->capturedVariables->value;
        for (uint8_t i = 0; i < c->capturedVariablesCount; i++) {
            stackSetVariable(c->argumentCount + i, cv[i], thread);
        }
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        thread->tokenStream = c->tokenStream;
        Something ret = runFunctionPointerBlock(thread, c->coinCount);
        thread->tokenStream = preCoinStream;
        
        stackPop(thread);
        return ret;
    }
}

Something performInitializer(Class *class, InitializerFunction *initializer, Object *object, Thread *thread){
    if(object == NULL){
        object = newObject(class);
    }
    
    if (initializer->native) {
        stackPush(somethingObject(object), initializer->argumentCount, initializer->argumentCount, thread);
        initializer->handler(thread);
        
        if(object->value == NULL){
            stackPop(thread);
            return NOTHINGNESS;
        }
    }
    else {
        stackPush(somethingObject(object), initializer->variableCount, initializer->argumentCount, thread);
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        
        thread->tokenStream = initializer->tokenStream;
        
        EmojicodeCoin *end = thread->tokenStream + initializer->tokenCount;
        while (thread->tokenStream < end) {
            parse(consumeCoin(thread), thread);
            
            if(thread->returned){
                thread->tokenStream = preCoinStream;
                stackPop(thread);
                return NOTHINGNESS;
            }
        }
        
        thread->tokenStream = preCoinStream;
    }
    object = stackGetThisObject(thread);
    stackPop(thread);

    return somethingObject(object);
}

Something performFunction(Function *method, Something this, Thread *thread){
    Something ret;
    if (method->native) {
        stackPush(this, method->argumentCount, method->argumentCount, thread);
        ret = method->handler(thread);
    }
    else {
        stackPush(this, method->variableCount, method->argumentCount, thread);
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        
        thread->tokenStream = method->tokenStream;
        
        ret = runFunctionPointerBlock(thread, method->tokenCount);
        
        thread->tokenStream = preCoinStream;
    }
    stackPop(thread);
    return ret;
}

Something parse(EmojicodeCoin coin, Thread *thread){
    switch (coin) {
        case 0x1: {
            Something sth = parse(consumeCoin(thread), thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            return performFunction(sth.object->class->methodsVtable[vti], sth, thread);
        }
        case 0x2: { //donut – class method
            Something sth = parse(consumeCoin(thread), thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            return performFunction(sth.eclass->methodsVtable[vti], sth, thread);
        }
        case 0x3: {
            Object *object = parse(consumeCoin(thread), thread).object;
        
            EmojicodeCoin pti = consumeCoin(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            return performFunction(object->class->protocolsTable[pti - object->class->protocolsOffset][vti],
                                   somethingObject(object), thread);
        }
        case 0x4: { //New Object
            Class *class = readClass(thread);
            
            InitializerFunction *initializer = class->initializersVtable[consumeCoin(thread)];
            return performInitializer(class, initializer, NULL, thread);
        }
        case 0x5: {
            Class *class = readClass(thread);
            EmojicodeCoin vti = consumeCoin(thread);
        
            return performFunction(class->methodsVtable[vti], stackGetThisContext(thread), thread);
        }
        case 0x6: {
            Something s = parse(consumeCoin(thread), thread);
            EmojicodeCoin c = consumeCoin(thread);
            return performFunction(functionTable[c], s, thread);
        }
        case 0x7: {
            EmojicodeCoin c = consumeCoin(thread);
            return performFunction(functionTable[c], NOTHINGNESS, thread);
        }
        case 0xE:
            return somethingClass(parse(consumeCoin(thread), thread).object->class);
        case 0xF:
            return somethingClass(classTable[consumeCoin(thread)]);
        case 0x10:
            return somethingObject(stringPool[consumeCoin(thread)]);
        case 0x11:
            return EMOJICODE_TRUE;
        case 0x12:
            return EMOJICODE_FALSE;
        case 0x13:
            return somethingInteger((EmojicodeInteger)(int)consumeCoin(thread));
        case 0x14: {
            EmojicodeInteger a = consumeCoin(thread);
            return somethingInteger(a << 32 | consumeCoin(thread));
        }
        case 0x15:
            return somethingDouble(readDouble(thread));
        case 0x16:
            return somethingSymbol((EmojicodeChar)consumeCoin(thread));
        case 0x17:
            return NOTHINGNESS;
        case 0x18:
            stackIncrementVariable(consumeCoin(thread), thread);
            return NOTHINGNESS;
        case 0x19:
            stackDecrementVariable(consumeCoin(thread), thread);
            return NOTHINGNESS;
        case 0x1A:
            return stackGetVariable(consumeCoin(thread), thread);
        case 0x1B: {
            EmojicodeCoin index = consumeCoin(thread);
            stackSetVariable(index, parse(consumeCoin(thread), thread), thread);
            return NOTHINGNESS;
        }
        case 0x1C: {
            EmojicodeCoin index = consumeCoin(thread);
            return objectGetVariable(stackGetThisObject(thread), index);
        }
        case 0x1D: {
            EmojicodeCoin index = consumeCoin(thread);
            objectSetVariable(stackGetThisObject(thread), index, parse(consumeCoin(thread), thread));
            return NOTHINGNESS;
        }
        case 0x1E: {
            EmojicodeCoin index = consumeCoin(thread);
            objectIncrementVariable(stackGetThisObject(thread), index);
            return NOTHINGNESS;
        }
        case 0x1F: {
            EmojicodeCoin index = consumeCoin(thread);
            objectDecrementVariable(stackGetThisObject(thread), index);
            return NOTHINGNESS;
        }
        //Operators
        case 0x20:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw == parse(consumeCoin(thread), thread).raw);
        case 0x21: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a - parse(consumeCoin(thread), thread).raw);
        }
        case 0x22: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a + parse(consumeCoin(thread), thread).raw);
        }
        case 0x23: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a * parse(consumeCoin(thread), thread).raw);
        }
        case 0x24: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a / parse(consumeCoin(thread), thread).raw);
        }
        case 0x25: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a % parse(consumeCoin(thread), thread).raw);
        }
        case 0x26: //Invert
            return !unwrapBool(parse(consumeCoin(thread), thread)) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
        case 0x27: {
            Something a = parse(consumeCoin(thread), thread);
            Something b = parse(consumeCoin(thread), thread);
            return unwrapBool(a) || unwrapBool(b) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
        }
        case 0x28: {
            Something a = parse(consumeCoin(thread), thread);
            Something b = parse(consumeCoin(thread), thread);
            return unwrapBool(a) && unwrapBool(b) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
        }
        //MARK: Integers
        case 0x29:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw < parse(consumeCoin(thread), thread).raw);
        case 0x2A:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw > parse(consumeCoin(thread), thread).raw);
        case 0x2B:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw <= parse(consumeCoin(thread), thread).raw);
        case 0x2C:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw >= parse(consumeCoin(thread), thread).raw);
        //MARK: General Comparisons
        case 0x2D:
            return somethingBoolean(parse(consumeCoin(thread), thread).object == parse(consumeCoin(thread), thread).object);
        case 0x2E:
            return isNothingness(parse(consumeCoin(thread), thread)) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
        //MARK: Floats
        case 0x2F:
            return somethingBoolean(parse(consumeCoin(thread), thread).doubl == parse(consumeCoin(thread), thread).doubl);
        case 0x30:
            return somethingDouble(parse(consumeCoin(thread), thread).doubl - parse(consumeCoin(thread), thread).doubl);
        case 0x31:
            return somethingDouble(parse(consumeCoin(thread), thread).doubl + parse(consumeCoin(thread), thread).doubl);
        case 0x32:
            return somethingDouble(parse(consumeCoin(thread), thread).doubl * parse(consumeCoin(thread), thread).doubl);
        case 0x33:
            return somethingDouble(parse(consumeCoin(thread), thread).doubl / parse(consumeCoin(thread), thread).doubl);
        case 0x34:
            return somethingBoolean(parse(consumeCoin(thread), thread).doubl < parse(consumeCoin(thread), thread).doubl);
        case 0x35:
            return somethingBoolean(parse(consumeCoin(thread), thread).doubl > parse(consumeCoin(thread), thread).doubl);
        case 0x36:
            return somethingBoolean(parse(consumeCoin(thread), thread).doubl <= parse(consumeCoin(thread), thread).doubl);
        case 0x37:
            return somethingBoolean(parse(consumeCoin(thread), thread).doubl >= parse(consumeCoin(thread), thread).doubl);
        case 0x38: {
            double a = parse(consumeCoin(thread), thread).doubl;
            double b = parse(consumeCoin(thread), thread).doubl;
            return somethingDouble(fmod(a, b));
        }
        //MARK: Optionals
        case 0x3A:
            return unwrapOptional(parse(consumeCoin(thread), thread));
        case 0x3B: {
            Something sth = parse(consumeCoin(thread), thread);
            EmojicodeCoin vti = consumeCoin(thread);
            EmojicodeCoin count = consumeCoin(thread);
            if(isNothingness(sth)){
                thread->tokenStream += count;
                return NOTHINGNESS;
            }
            
            Function *method = sth.object->class->methodsVtable[vti];
            return performFunction(method, sth, thread);
        }
        //MARK: Object Orientation Utility
        case 0x3C:
            return stackGetThisContext(thread);
        case 0x3D: {
            Class *class = readClass(thread);
            Object *o = stackGetThisObject(thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            InitializerFunction *initializer = class->initializersVtable[vti];
            
            performInitializer(class, initializer, o, thread);
            
            return NOTHINGNESS;
        }
        case 0x3E: {
            EmojicodeCoin index = consumeCoin(thread);
            Something sth = parse(consumeCoin(thread), thread);
            if (isNothingness(sth)) {
                return EMOJICODE_FALSE;
            }
            else {
                stackSetVariable(index, sth, thread);
                return EMOJICODE_TRUE;
            }
        }
        //MARK: Int To Double
        case 0x3F:
            return somethingDouble((double) parse(consumeCoin(thread), thread).raw);
        //MARK: Casts
        case 0x40: {
            Something sth = parse(consumeCoin(thread), thread);
            Class *class = readClass(thread);
            if(sth.type == T_OBJECT && instanceof(sth.object, class)){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x41: {
            Something sth = parse(consumeCoin(thread), thread);
            EmojicodeCoin pi = consumeCoin(thread);
            if(sth.type == T_OBJECT && conformsTo(sth.object->class, pi)){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x42: {
            Something sth = parse(consumeCoin(thread), thread);
            if(sth.type == T_BOOLEAN){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x43: {
            Something sth = parse(consumeCoin(thread), thread);
            if(sth.type == T_INTEGER){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x44: {
            Something sth = parse(consumeCoin(thread), thread);
            Class *class = readClass(thread);
            if(sth.type == T_OBJECT && !isNothingness(sth) && instanceof(sth.object, class)){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x45: {
            Something sth = parse(consumeCoin(thread), thread);
            EmojicodeCoin pi = consumeCoin(thread);
            if(sth.type == T_OBJECT && !isNothingness(sth) && conformsTo(sth.object->class, pi)){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x46: {
            Something sth = parse(consumeCoin(thread), thread);
            if(sth.type == T_SYMBOL){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case 0x47: {
            Something sth = parse(consumeCoin(thread), thread);
            if(sth.type == T_DOUBLE){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        //MARK: Literals
        case 0x50: {
            Object *dico = newObject(CL_DICTIONARY);
            stackPush(somethingObject(dico), 0, 0, thread);
            dictionaryInit(thread);
            stackPop(thread);
            Something *t = stackReserveFrame(NOTHINGNESS, 1, thread);
            *t = somethingObject(dico);
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end) {
                Object *key = parse(consumeCoin(thread), thread).object;
                Something sth = parse(consumeCoin(thread), thread);
                
                dictionarySet(t->object, key, sth, thread);
            }
            Something sth = *t;
            stackPushReservedFrame(thread);
            stackPop(thread);
            return sth;
        }
        case 0x51: {
            Something *t = stackReserveFrame(NOTHINGNESS, 1, thread);
            *t = somethingObject(newObject(CL_LIST));
            
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end) {
                listAppend(t->object, parse(consumeCoin(thread), thread), thread);
            }
            
            Something sth = *t;
            stackPushReservedFrame(thread);
            stackPop(thread);
            return sth;
        }
        case 0x52: {
            EmojicodeCoin stringCount = consumeCoin(thread);
            Something *t = stackReserveFrame(NOTHINGNESS, stringCount + 1, thread);
            
            EmojicodeInteger length = 0;
            
            for (EmojicodeCoin i = 0; i < stringCount; i++) {
                Something sm = parse(consumeCoin(thread), thread);
                t[i] = sm;
                String *string = sm.object->value;
                length += string->length;
            }
            
            stackPushReservedFrame(thread);
            
            Object *object = newObject(CL_STRING);
            
            stackSetVariable(stringCount, somethingObject(object), thread);
            
            Object *characters = newArray(length * sizeof(EmojicodeChar));
            EmojicodeChar *chars = characters->value;
            EmojicodeChar *writeChars = chars;
            
            Something sm = stackGetVariable(stringCount, thread);
            String *string = sm.object->value;
            
            for (int i = 0; i < stringCount; i++) {
                Object *o = stackGetVariable(i, thread).object;
                String *string = o->value;
                memcpy(writeChars, string->characters->value, string->length * sizeof(EmojicodeChar));
                writeChars += string->length;
            }
            
            string->length = length;
            string->characters = characters;
            
            stackPop(thread);
            
            return sm;
        }
        case 0x53: {
            EmojicodeInteger start = parse(consumeCoin(thread), thread).raw;
            EmojicodeInteger stop = parse(consumeCoin(thread), thread).raw;
            Object *object = newObject(CL_RANGE);
            EmojicodeRange *range = object->value;
            range->start = start;
            range->stop = stop;
            rangeSetDefaultStep(range);
            return somethingObject(object);
        }
        case 0x54: {
            EmojicodeInteger start = parse(consumeCoin(thread), thread).raw;
            EmojicodeInteger stop = parse(consumeCoin(thread), thread).raw;
            EmojicodeInteger step = parse(consumeCoin(thread), thread).raw;
            Object *object = newObject(CL_RANGE);
            EmojicodeRange *range = object->value;
            range->start = start;
            range->stop = stop;
            range->step = step;
            if (range->step == 0) rangeSetDefaultStep(range);
            return somethingObject(object);
        }
        //MARK: Binary Operations
        case 0x5A:
            return somethingInteger(parse(consumeCoin(thread), thread).raw & parse(consumeCoin(thread), thread).raw);
        case 0x5B:
            return somethingInteger(parse(consumeCoin(thread), thread).raw | parse(consumeCoin(thread), thread).raw);
        case 0x5C:
            return somethingInteger(parse(consumeCoin(thread), thread).raw ^ parse(consumeCoin(thread), thread).raw);
        case 0x5D:
            return somethingInteger(~parse(consumeCoin(thread), thread).raw);
        case 0x5E: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a << parse(consumeCoin(thread), thread).raw);
        }
        case 0x5F: {
            EmojicodeInteger a = parse(consumeCoin(thread), thread).raw;
            return somethingInteger(a >> parse(consumeCoin(thread), thread).raw);
        }
        //MARK: Flow Control
        case 0x60: { //Red apple - return
            thread->returnValue = parse(consumeCoin(thread), thread);
            thread->returned = true;
            return NOTHINGNESS;
        }
        case 0x61: { //MARK: cherries
            EmojicodeCoin *beginPosition = thread->tokenStream;
            while (unwrapBool(parse(consumeCoin(thread), thread))) {
                if (runBlock(thread)) {
                    return NOTHINGNESS;
                }
                thread->tokenStream = beginPosition;
            }
            passBlock(thread);
            return NOTHINGNESS;
        }
        case 0x62: { //MARK: If
            EmojicodeCoin length = consumeCoin(thread);
            EmojicodeCoin *ifEnd = thread->tokenStream + length;
            
            Something boolSth = parse(consumeCoin(thread), thread);
            if (unwrapBool(boolSth)) {  // Main if
                if (runBlock(thread)) {
                    return NOTHINGNESS;
                }
                thread->tokenStream = ifEnd;
            }
            else {
                passBlock(thread);
                
                while (thread->tokenStream < ifEnd && nextCoin(thread) == 0x63) {  // All else ifs
                    consumeCoin(thread);
                    
                    boolSth = parse(consumeCoin(thread), thread);
                    if (unwrapBool(boolSth)) {
                        if (runBlock(thread)) {
                            return NOTHINGNESS;
                        }
                        thread->tokenStream = ifEnd;
                        return NOTHINGNESS;
                    }
                    else {
                        passBlock(thread);
                    }
                }
                
                if (thread->tokenStream < ifEnd) {  // Else block
                    if (runBlock(thread)) {
                        return NOTHINGNESS;
                    }
                }
            }
            return NOTHINGNESS;
        }
        case 0x64: { //MARK: foreach
            //The destination variable
            EmojicodeCoin variable = consumeCoin(thread);
            
            Something iteratee = parse(consumeCoin(thread), thread);
            
            Something enumerator = performFunction(iteratee.object->class->protocolsTable[1 - iteratee.object->class->protocolsOffset][0], iteratee, thread);
            EmojicodeCoin enumeratorVindex = consumeCoin(thread);
            stackSetVariable(enumeratorVindex, enumerator, thread);
            
            Function *nextMethod = enumerator.object->class->protocolsTable[0][0];
            Function *moreComing = enumerator.object->class->protocolsTable[0][1];
            
            EmojicodeCoin *begin = thread->tokenStream;
            
            while (unwrapBool(performFunction(moreComing, stackGetVariable(enumeratorVindex, thread), thread))) {
                stackSetVariable(variable, performFunction(nextMethod, stackGetVariable(enumeratorVindex, thread), thread), thread);
                
                if(runBlock(thread)){
                    return NOTHINGNESS;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            
            return NOTHINGNESS;
        }
        case 0x65: { //MARK: foreach for lists
            //The destination variable
            EmojicodeCoin variable = consumeCoin(thread);
            
            //Get the list
            Something losm = parse(consumeCoin(thread), thread);
            
            EmojicodeCoin listObjectVariable = consumeCoin(thread);
            stackSetVariable(listObjectVariable, losm, thread);
            List *list = losm.object->value;
            
            EmojicodeCoin *begin = thread->tokenStream;
            
            for (size_t i = 0; i < (list = stackGetVariable(listObjectVariable, thread).object->value)->count; i++) {
                stackSetVariable(variable, unwrapOptional(listGet(list, i)), thread);
                
                if(runBlock(thread)){
                    return NOTHINGNESS;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            
            return NOTHINGNESS;
        }
        case 0x66: {
            EmojicodeCoin variable = consumeCoin(thread);
            EmojicodeRange range = *(EmojicodeRange *)parse(consumeCoin(thread), thread).object->value;
            EmojicodeCoin *begin = thread->tokenStream;
            for (EmojicodeInteger i = range.start; i != range.stop; i += range.step) {
                stackSetVariable(variable, somethingInteger(i), thread);
                
                if(runBlock(thread)){
                    return NOTHINGNESS;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            
            return NOTHINGNESS;
        }
        case 0x70: {
            Object *callable = parse(consumeCoin(thread), thread).object;
            if (callable->class == CL_CAPTURED_FUNCTION_CALL) {
                CapturedFunctionCall *cmc = callable->value;
                return performFunction(cmc->function, cmc->callee, thread);
            }
            else {
                Closure *c = callable->value;
                stackPush(c->thisContext, c->variableCount, c->argumentCount, thread);
                
                Something *cv = c->capturedVariables->value;
                for (uint8_t i = 0; i < c->capturedVariablesCount; i++) {
                    stackSetVariable(c->argumentCount + i, cv[i], thread);
                }
                
                EmojicodeCoin *preCoinStream = thread->tokenStream;
                thread->tokenStream = c->tokenStream;
                Something ret = runFunctionPointerBlock(thread, c->coinCount);
                thread->tokenStream = preCoinStream;
                
                stackPop(thread);
                return ret;
            }
        }
        case 0x71: {
            stackPush(stackGetThisContext(thread), 1, 0, thread);
            stackSetVariable(0, somethingObject(newObject(CL_CLOSURE)), thread);
            
            Object *co = stackGetVariable(0, thread).object;
            Closure *c = co->value;
            
            c->variableCount = consumeCoin(thread);
            c->coinCount = consumeCoin(thread);
            c->tokenStream = thread->tokenStream;
            thread->tokenStream += c->coinCount;
            
            EmojicodeCoin argumentCount = consumeCoin(thread);
            c->argumentCount = argumentCount;
            
            stackPop(thread);
            
            c->capturedVariablesCount = consumeCoin(thread);
            
            Object *capturedVariables = newArray(sizeof(Something) * c->capturedVariablesCount);
            c->capturedVariables = capturedVariables;
            
            Something *t = capturedVariables->value;
            for (uint_fast8_t i = 0; i < c->capturedVariablesCount; i++) {
                t[i] = stackGetVariable(i, thread);
            }
            
            if (argumentCount >> 16)
                c->thisContext = stackGetThisContext(thread);
            
            return somethingObject(co);
        }
        case 0x72: {
            stackPush(parse(consumeCoin(thread), thread), 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = stackGetThisObject(thread)->class->methodsVtable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            return somethingObject(cmco);
        }
        case 0x73: {
            stackPush(parse(consumeCoin(thread), thread), 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = stackGetThisContext(thread).eclass->methodsVtable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            return somethingObject(cmco);
        }
        case 0x74: {
            stackPush(parse(consumeCoin(thread), thread), 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = functionTable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            return somethingObject(cmco);
        }
    }
    return NOTHINGNESS;
}

int main(int argc, char *argv[]) {
    cliArgumentCount = argc;
    cliArguments = argv;
    const char *ppath;
    if ((ppath = getenv("EMOJICODE_PACKAGES_PATH"))) {
        packageDirectory = ppath;
    }
    
    setlocale(LC_CTYPE, "de_DE.UTF-8");
    if (argc < 2){
       error("No file provided.");
    }
    
    FILE *f = fopen(argv[1], "rb");
    if (!f || ferror(f)){
       error("File couldn't be opened.");
    }
    
    Thread *mainThread = allocateThread();
    
    allocateHeap();
    
    Function *handler = readBytecode(f);
    return (int)performFunction(handler, NOTHINGNESS, mainThread).raw;
}
