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
#include "EmojicodeInstructions.h"

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

Something garbage;

//MARK: Coins

EmojicodeCoin consumeCoin(Thread *thread) {
    return *(thread->tokenStream++);
}

static EmojicodeCoin nextCoin(Thread *thread) {
    return *(thread->tokenStream);
}

//MARK: Error

_Noreturn void error(char *err, ...) {
    va_list list;
    va_start(list, err);
    
    char error[350];
    vsprintf(error, err, list);
    
    fprintf(stderr, "🚨 Fatal Error: %s\n", error);
    
    va_end(list);
    exit(1);
}

//MARK: Block utilities

static void passBlock(Thread *thread) {
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
        produce(consumeCoin(thread), thread, &garbage);
    
        pauseForGC(NULL);
        
        if (thread->tokenStream == NULL) {
            return true;
        }
    }
    
    return false;
}

static void runFunctionPointerBlock(Thread *thread, uint32_t length){
    EmojicodeCoin *end = thread->tokenStream + length;
    while (thread->tokenStream < end) {
        produce(consumeCoin(thread), thread, &garbage);
        
        pauseForGC(NULL);
        
        if (thread->tokenStream == NULL) {
            return;
        }
    }
    return;
}

Class* readClass(Thread *thread) {
    return evaluateExpression(consumeCoin(thread), thread).eclass;
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

void unwrapOptional(Something sth) {
    if (isNothingness(sth)) {
        error("Unexpectedly found ✨ while unwrapping a 🍬.");
    }
}

//MARK: Low level parsing

void executeCallableExtern(Object *callable, Something *args, Thread *thread, Something *destination){
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
            thread->returnDestination = destination;
            runFunctionPointerBlock(thread, method->tokenCount);
            thread->tokenStream = preCoinStream;
        }
        stackPop(thread);
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
        thread->returnDestination = destination;
        runFunctionPointerBlock(thread, c->coinCount);
        thread->tokenStream = preCoinStream;
        
        stackPop(thread);
    }
}

Something performInitializer(Class *class, InitializerFunction *initializer, Object *object, Thread *thread) {
    if (object == NULL) {
        object = newObject(class);
    }
    
    if (initializer->native) {
        stackPush(somethingObject(object), initializer->argumentCount, initializer->argumentCount, thread);
        initializer->handler(thread);
        
        if (object->value == NULL) {
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
            produce(consumeCoin(thread), thread, &garbage);
            
            if (thread->returned) {
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

void performFunction(Function *method, Something this, Thread *thread, Something *destination) {
    Something ret;
    if (method->native) {
        stackPush(this, method->argumentCount, method->argumentCount, thread);
        ret = method->handler(thread); // TODO: Wrong
    }
    else {
        stackPush(this, method->variableCount, method->argumentCount, thread);
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        
        thread->returnDestination = destination;
        thread->tokenStream = method->tokenStream;
        runFunctionPointerBlock(thread, method->tokenCount);
        thread->tokenStream = preCoinStream;
    }
    stackPop(thread);
}

void produce(EmojicodeCoin coin, Thread *thread, Something *destination) {
    switch ((EmojicodeInstruction)coin) {
        case INS_DISPATCH_METHOD: {
            Something sth = evaluateExpression(consumeCoin(thread), thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            performFunction(sth.object->class->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_TYPE_METHOD: { //donut – class method
            Something sth = evaluateExpression(consumeCoin(thread), thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            performFunction(sth.eclass->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_PROTOCOL: {
            Object *object = evaluateExpression(consumeCoin(thread), thread).object;
            
            EmojicodeCoin pti = consumeCoin(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            performFunction(object->class->protocolsTable[pti - object->class->protocolsOffset][vti],
                            somethingObject(object), thread, destination);
            return;
        }
        case INS_NEW_OBJECT: { //New Object
            Class *class = readClass(thread);
            
            InitializerFunction *initializer = class->initializersVtable[consumeCoin(thread)];
            performInitializer(class, initializer, NULL, thread);
            return;
        }
        case INS_DISPATCH_SUPER: {
            Class *class = readClass(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            performFunction(class->methodsVtable[vti], stackGetThisContext(thread), thread, destination);
            return;
        }
        case INS_CALL_CONTEXTED_FUNCTION: {
            Something s = parse(consumeCoin(thread), thread);
            EmojicodeCoin c = consumeCoin(thread);
            return performFunction(functionTable[c], s, thread);
        }
        case INS_CALL_FUNCTION: {
            EmojicodeCoin c = consumeCoin(thread);
            performFunction(functionTable[c], NOTHINGNESS, thread, destination);
            return;
        }
        case INS_GET_CLASS_FROM_INSTANCE: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = somethingClass(a.object->class);
            return;
        }
        case INS_GET_CLASS_FROM_INDEX:
            *destination = somethingClass(classTable[consumeCoin(thread)]);
            return;
        case INS_GET_STRING_POOL:
            *destination = somethingObject(stringPool[consumeCoin(thread)]);
            return;
        case INS_GET_TRUE:
            *destination = EMOJICODE_TRUE;
            return;
        case INS_GET_FALSE:
            *destination = EMOJICODE_FALSE;
            return;
        case INS_GET_32_INTEGER:
            *destination = somethingInteger((EmojicodeInteger)(int)consumeCoin(thread));
            return;
        case INS_GET_64_INTEGER: {
            EmojicodeInteger a = consumeCoin(thread);
            *destination = somethingInteger(a << 32 | consumeCoin(thread));
            return;
        }
        case INS_GET_DOUBLE:
            *destination = somethingDouble(readDouble(thread));
            return;
        case INS_GET_SYMBOL:
            *destination = somethingSymbol((EmojicodeChar)consumeCoin(thread));
            return;
        case INS_GET_NOTHINGNESS:
            *destination = NOTHINGNESS;
            return;
        case INS_PRODUCE_WITH_STACK_DESTINATION: {
            EmojicodeCoin index = consumeCoin(thread);
            Something *d = (Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index);
            produce(consumeCoin(thread), thread, d);
            return;
        }
        case INS_PRODUCE_WITH_INSTANCE_DESTINATION: {
            EmojicodeCoin index = consumeCoin(thread);
            Object *o = ((StackFrame *)thread->stack)->thisContext.object;
            Something *d = (Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index);
            produce(consumeCoin(thread), thread, d);
            return;
        }
        case INS_INCREMENT:
            destination->raw++;
            return;
        case INS_DECREMENT:
            destination->raw--;
            return;
        case INS_COPY_SINGLE_STACK: {
            EmojicodeCoin index = consumeCoin(thread);
            *destination = *(Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index);
            return;
        }
        case INS_COPY_WITH_SIZE_STACK: {
            EmojicodeCoin index = consumeCoin(thread);
            Something *source = (Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index);
            memcpy(destination, source, sizeof(Something) * consumeCoin(thread));
            return;
        }
        case INS_COPY_SINGLE_INSTANCE: {
            EmojicodeCoin index = consumeCoin(thread);
            Object *o = ((StackFrame *)thread->stack)->thisContext.object;
            *destination = *(Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index);
        }
        case INS_COPY_WITH_SIZE_INSTANCE: {
            EmojicodeCoin index = consumeCoin(thread);
            Object *o = ((StackFrame *)thread->stack)->thisContext.object;
            Something *source = (Something *)(((Byte *)o) + sizeof(Object) + sizeof(Something) * index);
            memcpy(destination, source, sizeof(Something) * consumeCoin(thread));
            return;
        }
        // Operators
        case INS_EQUAL_PRIMITIVE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw == b.raw);
            return;
        }
        case INS_SUBTRACT_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw - b.raw);
            return;
        }
        case INS_ADD_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw + b.raw);
            return;
        }
        case INS_MULTIPLY_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw * b.raw);
            return;
        }
        case INS_DIVIDE_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw / b.raw);
            return;
        }
        case INS_REMAINDER_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw % b.raw);
            return;
        }
        case INS_INVERT_BOOLEAN: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = !unwrapBool(a) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
            return;
        }
        case INS_OR_BOOLEAN: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = unwrapBool(a) || unwrapBool(b) ? EMOJICODE_TRUE : EMOJICODE_FALSE;;
            return;
        }
        case INS_AND_BOOLEAN: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = unwrapBool(a) && unwrapBool(b) ? EMOJICODE_TRUE : EMOJICODE_FALSE;;
            return;
        }
        case INS_LESS_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw < b.raw);
            return;
        }
        case INS_GREATER_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw > b.raw);
            return;
        }
        case INS_GREATER_OR_EQUAL_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw <= b.raw);
            return;
        }
        case INS_LESS_OR_EQUAL_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw >= b.raw);
            return;
        }
        case INS_SAME_OBJECT: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.object == b.object);
            return;
        }
        case INS_IS_NOTHINGNESS: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = isNothingness(a) ? EMOJICODE_TRUE : EMOJICODE_FALSE;
            return;
        }
        case INS_EQUAL_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl == b.doubl);
            return;
        }
        case INS_SUBTRACT_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingDouble(a.doubl - b.doubl);
            return;
        }
        case INS_ADD_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingDouble(a.doubl + b.doubl);
            return;
        }
        case INS_MULTIPLY_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingDouble(a.doubl * b.doubl);
            return;
        }
        case INS_DIVIDE_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl / b.doubl);
            return;
        }
        case INS_LESS_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl < b.doubl);
            return;
        }
        case INS_GREATER_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl > b.doubl);
            return;
        }
        case INS_LESS_OR_EQUAL_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl <= b.doubl);
            return;
        }
        case INS_GREATER_OR_EQUAL_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.doubl >= b.doubl);
            return;
        }
        case INS_REMAINDER_DOUBLE: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingDouble(fmod(a.doubl, b.doubl));
        }
        case INS_INT_TO_DOUBLE: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = somethingDouble((double) a.raw);
        }
        case INS_UNWRAP_OPTIONAL:
            produce(consumeCoin(thread), thread, destination);
            unwrapOptional(*destination);
            return;
        case 0x3B: {
            Something sth = parse(consumeCoin(thread), thread);
            EmojicodeCoin vti = consumeCoin(thread);
            EmojicodeCoin count = consumeCoin(thread);
            if (isNothingness(sth)) {
                thread->tokenStream += count;
                return NOTHINGNESS;
            }
            
            Function *method = sth.object->class->methodsVtable[vti];
            return performFunction(method, sth, thread);
        }
            //MARK: Object Orientation Utility
        case 0x3C:
            *destination = stackGetThisContext(thread);
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
            produce(consumeCoin(thread), thread, index);
            if (isNothingness(sth)) {
                *destination = EMOJICODE_FALSE;
            }
            else {
                *destination = EMOJICODE_TRUE;
            }
            return;
        }
            //MARK: Casts
        case INS_CAST_TO_CLASS: {
            produce(consumeCoin(thread), thread, destination);
            Class *class = readClass(thread);
            if (!(destination->type == T_OBJECT && instanceof(destination->object, class))){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_PROTOCOL: {
            produce(consumeCoin(thread), thread, destination);
            EmojicodeCoin pi = consumeCoin(thread);
            if (!(destination->type == T_OBJECT && conformsTo(destination->object->class, pi))){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_BOOLEAN: {
            produce(consumeCoin(thread), thread, destination);
            if (destination->type != T_BOOLEAN){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_INTEGER: {
            Something sth = parse(consumeCoin(thread), thread);
            if(sth.type == T_INTEGER){
                return sth;
            }
            
            return NOTHINGNESS;
        }
        case INS_SAFE_CAST_TO_CLASS: {
            produce(consumeCoin(thread), thread, destination);
            Class *class = readClass(thread);
            if (!(destination->type == T_OBJECT && !isNothingness(destination)
                  && instanceof(destination->object, class))){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_SAFE_CAST_TO_PROTOCOL: {
            produce(consumeCoin(thread), thread, destination);
            EmojicodeCoin pi = consumeCoin(thread);
            if (!(destination->type == T_OBJECT && !isNothingness(destination) &&
                  conformsTo(destination->object->class, pi))){
                *destination = NOTHINGNESS;
            }
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
    error("Illegal bytecode instruction");
}

Something evaluateExpression(EmojicodeCoin coin, Thread *thread) {
    
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
