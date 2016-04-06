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
Class *CL_ENUMERATOR;
Class *CL_DICTIONARY;
Class *CL_CAPTURED_METHOD_CALL;
Class *CL_CLOSURE;
Class *CL_RANGE;

char **cliArguments;
int cliArgumentCount;

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

static Something runFunctionBlock(Thread *thread, uint32_t length){
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
    EmojicodeCoin classIndex = consumeCoin(thread);
    
    if(classIndex == UINT32_MAX){
        return stackGetThisClass(thread);
    }
    
    return classTable[classIndex];
}

static double readDouble(Thread *thread) {
    EmojicodeInteger scale = ((EmojicodeInteger)consumeCoin(thread) << 32) ^ consumeCoin(thread);
    EmojicodeInteger exp = consumeCoin(thread);
    
    return ldexp((double)scale/PORTABLE_INTLEAST64_MAX, (int)exp);
}


//MARK:

bool isNothingness(Something sth){
    return sth.type == T_OBJECT && sth.object == NULL;
}

bool isRealObject(Something sth){
    return sth.type == T_OBJECT && sth.object;
}

//MARK: Low level parsing

Something executeCallableExtern(Object *callable, Something *args, Thread *thread){
    if (callable->class == CL_CAPTURED_METHOD_CALL) {
        CapturedMethodCall *cmc = callable->value;
        Method *method = cmc->method;
        Object *object = cmc->object;
        
        Something ret;
        if (method->native) {
            Something *t = stackReserveFrame(object, method->argumentCount, thread);
            memcpy(t, args, method->argumentCount * sizeof(Something));
            stackPushReservedFrame(thread);
            ret = method->handler(thread);
        }
        else {
            Something *t = stackReserveFrame(object, method->variableCount, thread);
            memcpy(t, args, method->argumentCount * sizeof(Something));
            stackPushReservedFrame(thread);
            
            EmojicodeCoin *preCoinStream = thread->tokenStream;
            
            thread->tokenStream = method->tokenStream;
            
            ret = runFunctionBlock(thread, method->tokenCount);
            
            thread->tokenStream = preCoinStream;
        }
        stackPop(thread);
        return ret;
    }
    else {
        Closure *c = callable->value;
        
        Something *t = stackReserveFrame(c->this, c->variableCount, thread);
        memcpy(t, args, c->argumentCount * sizeof(Something));
        stackPushReservedFrame(thread);
        
        Something *cv = c->capturedVariables->value;
        for (uint8_t i = 0; i < c->capturedVariablesCount; i++) {
            stackSetVariable(c->argumentCount + i, cv[i], thread);
        }
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        thread->tokenStream = c->tokenStream;
        Something ret = runFunctionBlock(thread, c->coinCount);
        thread->tokenStream = preCoinStream;
        
        stackPop(thread);
        return ret;
    }
}

Something performInitializer(Class *class, Initializer *initializer, Object *object, Thread *thread){
    if(object == NULL){
        object = newObject(class);
    }
    
    if (initializer->native) {
        stackPush(object, initializer->argumentCount, initializer->argumentCount, thread);
        initializer->handler(thread);
        
        if(object->value == NULL){
            stackPop(thread);
            return NOTHINGNESS;
        }
    }
    else {
        stackPush(object, initializer->variableCount, initializer->argumentCount, thread);
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
    stackPop(thread);

    return somethingObject(object);
}

Something performMethod(Method *method, Object *object, Thread *thread){
    Something ret;
    if (method->native) {
        stackPush(object, method->argumentCount, method->argumentCount, thread);
        ret = method->handler(thread);
    }
    else {
        stackPush(object, method->variableCount, method->argumentCount, thread);
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        
        thread->tokenStream = method->tokenStream;
        
        ret = runFunctionBlock(thread, method->tokenCount);
        
        thread->tokenStream = preCoinStream;
    }
    stackPop(thread);
    
    return ret;
}

Something performClassMethod(ClassMethod *method, Class *class, Thread *thread){
    Something ret;
    if (method->native) {
        stackPush(class, method->argumentCount, method->argumentCount, thread);
        ret = method->handler(thread);
    }
    else {
        stackPush(class, method->variableCount, method->argumentCount, thread);
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        
        thread->tokenStream = method->tokenStream;
        
        ret = runFunctionBlock(thread, method->tokenCount);
        
        thread->tokenStream = preCoinStream;
    }
    stackPop(thread);
    
    return ret;
}


Something parse(EmojicodeCoin coin, Thread *thread){
    switch (coin) {
        case 0x1: {
            Object *object = parse(consumeCoin(thread), thread).object;
            
            EmojicodeCoin vti = consumeCoin(thread);
            return performMethod(object->class->methodsVtable[vti], object, thread);
        }
        case 0x2: { //donut – class method
            Class *class = readClass(thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            return performClassMethod(class->classMethodsVtable[vti], class, thread);
        }
        case 0x3: {
            Object *object = parse(consumeCoin(thread), thread).object;
            
            EmojicodeCoin pti = consumeCoin(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            return performMethod(object->class->protocolsTable[pti - object->class->protocolsOffset][vti], object, thread);
        }
        case 0x4: { //New Object
            Class *class = readClass(thread);
            
            Initializer *initializer = class->initializersVtable[consumeCoin(thread)];
            return performInitializer(class, initializer, NULL, thread);
        }
        case 0x5: {
            Class *class = readClass(thread);
            EmojicodeCoin vti = consumeCoin(thread);
        
            return performMethod(class->methodsVtable[vti], stackGetThis(thread), thread);
        }
        case 0x10:
            return somethingObject(stringPool[consumeCoin(thread)]);
        case 0x11:
            return EMOJICODE_TRUE;
        case 0x12:
            return EMOJICODE_FALSE;
        case 0x13:
            return somethingInteger((EmojicodeInteger)(int)consumeCoin(thread));
        case 0x14:
            return somethingInteger((EmojicodeInteger)consumeCoin(thread) << 32 | consumeCoin(thread));
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
            return objectGetVariable(stackGetThis(thread), index);
        }
        case 0x1D: {
            EmojicodeCoin index = consumeCoin(thread);
            objectSetVariable(stackGetThis(thread), index, parse(consumeCoin(thread), thread));
            return NOTHINGNESS;
        }
        case 0x1E: {
            EmojicodeCoin index = consumeCoin(thread);
            objectIncrementVariable(stackGetThis(thread), index);
            return NOTHINGNESS;
        }
        case 0x1F: {
            EmojicodeCoin index = consumeCoin(thread);
            objectDecrementVariable(stackGetThis(thread), index);
            return NOTHINGNESS;
        }
        //Operators
        case 0x20:
            return somethingBoolean(parse(consumeCoin(thread), thread).raw == parse(consumeCoin(thread), thread).raw);
        case 0x21:
            return somethingInteger(parse(consumeCoin(thread), thread).raw - parse(consumeCoin(thread), thread).raw);
        case 0x22:
            return somethingInteger(parse(consumeCoin(thread), thread).raw + parse(consumeCoin(thread), thread).raw);
        case 0x23:
            return somethingInteger(parse(consumeCoin(thread), thread).raw * parse(consumeCoin(thread), thread).raw);
        case 0x24:
            return somethingInteger(parse(consumeCoin(thread), thread).raw / parse(consumeCoin(thread), thread).raw);
        case 0x25:
            return somethingInteger(parse(consumeCoin(thread), thread).raw % parse(consumeCoin(thread), thread).raw);
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
        //MARK: Optionals
        case 0x3A: {
            Something sth = parse(consumeCoin(thread), thread);
            
            if(isNothingness(sth)){
                error("Unexpectedly found ✨ while unwrapping a 🍬.");
            }
            
            return sth;
        }
        case 0x3B: {
            EmojicodeCoin count = consumeCoin(thread);
            Something sth = parse(consumeCoin(thread), thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            if(isNothingness(sth)){
                thread->tokenStream += count;
                return NOTHINGNESS;
            }
            
            Method *method = sth.object->class->methodsVtable[vti];
            Object *object = sth.object;
            
            return performMethod(method, object, thread);
        }
        //MARK: Object Orientation Utility
        case 0x3C:
            return somethingObject(stackGetThis(thread));
        case 0x3D: {
            Object *o = stackGetThis(thread);
            
            Class *class = readClass(thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            Initializer *initializer = class->initializersVtable[vti];
            
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
            stackPush(newObject(CL_DICTIONARY), 0, 0, thread);
            dictionaryInit(thread);
            
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end){
                Object *key = parse(consumeCoin(thread), thread).object;
                Something sth = parse(consumeCoin(thread), thread);
                
                dictionarySet(stackGetThis(thread), key, sth, thread);
            }
            
            Object *dict = stackGetThis(thread);
            stackPop(thread);
            
            return somethingObject(dict);
        }
        case 0x51: {
            Something *t = stackReserveFrame(NULL, 1, thread);
            
            t[0] = somethingObject(newObject(CL_LIST));
            
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end){
                listAppend(t[0].object, parse(consumeCoin(thread), thread), thread);
            }
            
            Something sth = t[0];
            
            stackPushReservedFrame(thread);
            stackPop(thread);
            
            return sth;
        }
        case 0x52: {
            EmojicodeCoin stringCount = consumeCoin(thread);
            Something *t = stackReserveFrame(NULL, stringCount + 1, thread);
            
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
        //MARK: Flow Control
        case 0x60: { //Red apple - return
            thread->returnValue = parse(consumeCoin(thread), thread);
            thread->returned = true;
            return NOTHINGNESS;
        }
        case 0x61: { //MARK: cherries
            EmojicodeCoin *beginPosition = thread->tokenStream;
            while (unwrapBool(parse(consumeCoin(thread), thread))) {
                if(runBlock(thread)) {
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
            bool b = unwrapBool(boolSth);
            
            if(b){
                //The if itself was true
                if(runBlock(thread)){
                    //We hit a return
                    return NOTHINGNESS;
                }
                thread->tokenStream = ifEnd;
            }
            else if(thread->tokenStream >= ifEnd){
                return NOTHINGNESS;
            }
            else {
                //Get away the 1st orange block
                passBlock(thread);
                
                while (thread->tokenStream < ifEnd && nextCoin(thread) == 0x1F34B) { //All else ifs
                    consumeCoin(thread);
                    
                    boolSth = parse(consumeCoin(thread), thread);
                    b = unwrapBool(boolSth);
                    
                    if (b) {
                        //Its condition is true, so let's execute
                        if(runBlock(thread)){
                            return NOTHINGNESS;
                        }
                        thread->tokenStream = ifEnd;
                        return NOTHINGNESS;
                    }
                    else {
                        passBlock(thread);
                    }
                }
                
                if(thread->tokenStream < ifEnd && nextCoin(thread) == 0x1F353){ //Else?
                    consumeCoin(thread);
                    
                    if(runBlock(thread)){
                        return NOTHINGNESS;
                    }
                }
            }
            return NOTHINGNESS;
        }
        case 0x64: { //MARK: foreach
            //The destination variable
            EmojicodeCoin variable = consumeCoin(thread);
            
            Object *iteratee = parse(consumeCoin(thread), thread).object;
            
            Something enumerator = performMethod(iteratee->class->protocolsTable[0][0], iteratee, thread);
            EmojicodeCoin enumeratorVindex = consumeCoin(thread);
            stackSetVariable(enumeratorVindex, enumerator, thread);
            
            Method *nextMethod = enumerator.object->class->methodsVtable[0];
            Method *moreComing = enumerator.object->class->methodsVtable[1];
            
            EmojicodeCoin *begin = thread->tokenStream;
            
            while (unwrapBool(performMethod(moreComing, stackGetVariable(enumeratorVindex, thread).object, thread))) {
                stackSetVariable(variable, performMethod(nextMethod, stackGetVariable(enumeratorVindex, thread).object, thread), thread);
                
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
                stackSetVariable(variable, listGet(list, i), thread);
                
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
            stackPush(stackGetThis(thread), 1, 0, thread);
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
                c->this = stackGetThis(thread);
            
            return somethingObject(co);
        }
        case 0x71: {
            stackPush(parse(consumeCoin(thread), thread).object, 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_METHOD_CALL);
            CapturedMethodCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->method = stackGetThis(thread)->class->methodsVtable[vti];
            cmc->object = stackGetThis(thread);
            stackPop(thread);
            return somethingObject(cmco);
        }
        case 0x72: {
            Object *callable = parse(consumeCoin(thread), thread).object;
            if (callable->class == CL_CAPTURED_METHOD_CALL) {
                CapturedMethodCall *cmc = callable->value;
                return performMethod(cmc->method, cmc->object, thread);
            }
            else {
                Closure *c = callable->value;
                stackPush(callable, c->variableCount, c->argumentCount, thread);
                
                Something *cv = c->capturedVariables->value;
                for (uint8_t i = 0; i < c->capturedVariablesCount; i++) {
                    stackSetVariable(c->argumentCount + i, cv[i], thread);
                }
                ((StackFrame*)thread->stack)->this = c->this;
                
                EmojicodeCoin *preCoinStream = thread->tokenStream;
                thread->tokenStream = c->tokenStream;
                Something ret = runFunctionBlock(thread, c->coinCount);
                thread->tokenStream = preCoinStream;
                
                stackPop(thread);
                return ret;
            }
        }
    }
    return NOTHINGNESS;
}

int main(int argc, char *argv[]) {
    cliArgumentCount = argc;
    cliArguments = argv;
    
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
    
    Class *cl;
    ClassMethod *flagMethod = readBytecode(f, &cl);

    return (int)performClassMethod(flagMethod, cl, mainThread).raw;
}
