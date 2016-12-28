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
    abort();
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
        produce(consumeCoin(thread), thread, NULL);
    
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
        produce(consumeCoin(thread), thread, NULL);
        
        pauseForGC(NULL);
        
        if (thread->tokenStream == NULL) {
            return;
        }
    }
    return;
}

Class* readClass(Thread *thread) {
    Something sth;
    produce(consumeCoin(thread), thread, &sth);
    return sth.eclass;
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

void loadCapture(Closure *c, Thread *thread) {
    Something *cv = c->capturedVariables->value;
    CaptureInformation *infop = c->capturesInformation->value;
    for (int i = 0; i < c->captureCount; i++) {
        memcpy(stackVariableDestination(infop->destination, thread), cv, infop->size * sizeof(Something));
        cv += infop->size;
        infop++;
    }
}

void executeCallableExtern(Object *callable, Something *args, Thread *thread, Something *destination) {
    if (callable->class == CL_CAPTURED_FUNCTION_CALL) {
        CapturedFunctionCall *cmc = callable->value;
        Function *method = cmc->function;

        if (method->native) {
            Something *t = stackReserveFrame(cmc->callee, method->argumentCount, thread);
            memcpy(t, args, method->argumentCount * sizeof(Something));
            stackPushReservedFrame(thread);
            method->handler(thread, destination);
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

        // TODO: Won’t work with real vts
        Something *t = stackReserveFrame(c->thisContext, c->variableCount, thread);
        memcpy(t, args, c->argumentCount * sizeof(Something));
        stackPushReservedFrame(thread);

        loadCapture(c, thread);
        
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
        Something *preReturnDest = thread->returnDestination;

        Something returned = EMOJICODE_TRUE;
        thread->tokenStream = initializer->tokenStream;
        thread->returnDestination = &returned;
        
        EmojicodeCoin *end = thread->tokenStream + initializer->tokenCount;
        while (thread->tokenStream < end) {
            produce(consumeCoin(thread), thread, NULL);
            
            if (!returned.raw) {
                thread->tokenStream = preCoinStream;
                stackPop(thread);
                return NOTHINGNESS;
            }
        }
        
        thread->tokenStream = preCoinStream;
        thread->returnDestination = preReturnDest;
    }
    object = stackGetThisObject(thread);
    stackPop(thread);

    return somethingObject(object);
}

void performFunction(Function *function, Something this, Thread *thread, Something *destination) {
    if (function->native) {
        stackPush(this, function->argumentCount, function->argumentCount, thread);
        function->handler(thread, destination);
    }
    else {
        stackPush(this, function->variableCount, function->argumentCount, thread);
        
        EmojicodeCoin *preCoinStream = thread->tokenStream;
        Something *preReturnDest = thread->returnDestination;

        thread->returnDestination = destination;
        thread->tokenStream = function->tokenStream;
        runFunctionPointerBlock(thread, function->tokenCount);
        thread->tokenStream = preCoinStream;
        thread->returnDestination = preReturnDest;
    }
    stackPop(thread);
}

void produce(EmojicodeCoin coin, Thread *thread, Something *destination) {
    switch ((EmojicodeInstruction)coin) {
        case INS_DISPATCH_METHOD: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);

            EmojicodeCoin vti = consumeCoin(thread);
            performFunction(sth.object->class->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_TYPE_METHOD: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);

            EmojicodeCoin vti = consumeCoin(thread);
            performFunction(sth.eclass->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_PROTOCOL: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            
            EmojicodeCoin pti = consumeCoin(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            performFunction(sth.object->class->protocolsTable[pti - sth.object->class->protocolsOffset][vti],
                            sth, thread, destination);
            return;
        }
        case INS_NEW_OBJECT: { //New Object
            Class *class = readClass(thread);
            
            InitializerFunction *initializer = class->initializersVtable[consumeCoin(thread)];
            *destination = performInitializer(class, initializer, NULL, thread);
            return;
        }
        case INS_DISPATCH_SUPER: {
            Class *class = readClass(thread);
            EmojicodeCoin vti = consumeCoin(thread);
            
            performFunction(class->methodsVtable[vti], stackGetThisContext(thread), thread, destination);
            return;
        }
        case INS_CALL_CONTEXTED_FUNCTION: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);

            EmojicodeCoin c = consumeCoin(thread);
            performFunction(functionTable[c], sth, thread, destination);
            return;
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
            produce(consumeCoin(thread), thread, stackVariableDestination(index, thread));
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
            return;
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
            *destination = somethingBoolean(a.raw >= b.raw);
            return;
        }
        case INS_LESS_OR_EQUAL_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingBoolean(a.raw <= b.raw);
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
            *destination = somethingDouble(a.doubl / b.doubl);
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
            return;
        }
        case INS_INT_TO_DOUBLE: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = somethingDouble((double) a.raw);
            return;
        }
        case INS_UNWRAP_OPTIONAL:
            produce(consumeCoin(thread), thread, destination);
            unwrapOptional(*destination);
            return;
        case 0x3B: {  // TODO: Won’t work with real vts
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            EmojicodeCoin vti = consumeCoin(thread);
            EmojicodeCoin count = consumeCoin(thread);
            if (isNothingness(sth)) {
                thread->tokenStream += count;
                return;
            }
            
            Function *method = sth.object->class->methodsVtable[vti];
            performFunction(method, sth, thread, destination);
            return;
        }
        case INS_GET_THIS:
            *destination = stackGetThisContext(thread);
            return;
        case INS_SUPER_INITIALIZER: {
            Class *class = readClass(thread);
            Object *o = stackGetThisObject(thread);
            
            EmojicodeCoin vti = consumeCoin(thread);
            InitializerFunction *initializer = class->initializersVtable[vti];
            
            performInitializer(class, initializer, o, thread);
            return;
        }
        case INS_CONDITIONAL_PRODUCE: {
            EmojicodeCoin index = consumeCoin(thread);
            Something *d = stackVariableDestination(index, thread);
            produce(consumeCoin(thread), thread, d);
            if (isNothingness(*d)) {
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
            if (!(destination->type == T_OBJECT && instanceof(destination->object, class))) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_PROTOCOL: {
            produce(consumeCoin(thread), thread, destination);
            EmojicodeCoin pi = consumeCoin(thread);
            if (!(destination->type == T_OBJECT && conformsTo(destination->object->class, pi))) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_BOOLEAN: {
            produce(consumeCoin(thread), thread, destination);
            if (destination->type != T_BOOLEAN) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_CAST_TO_INTEGER: {
            produce(consumeCoin(thread), thread, destination);
            if(destination->type != T_INTEGER) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_SAFE_CAST_TO_CLASS: {
            produce(consumeCoin(thread), thread, destination);
            Class *class = readClass(thread);
            if (!(destination->type == T_OBJECT && !isNothingness(*destination)
                  && instanceof(destination->object, class))){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case INS_SAFE_CAST_TO_PROTOCOL: {
            produce(consumeCoin(thread), thread, destination);
            EmojicodeCoin pi = consumeCoin(thread);
            if (!(destination->type == T_OBJECT && !isNothingness(*destination) &&
                  conformsTo(destination->object->class, pi))){
                *destination = NOTHINGNESS;
            }
            return;
        }
        case 0x46: {
            produce(consumeCoin(thread), thread, destination);
            if(destination->type != T_SYMBOL) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case 0x47: {
            produce(consumeCoin(thread), thread, destination);
            if(destination->type != T_DOUBLE) {
                *destination = NOTHINGNESS;
            }
            return;
        }
        case 0x50: {
            Object *dico = newObject(CL_DICTIONARY);
            stackPush(somethingObject(dico), 0, 0, thread);
            dictionaryInit(thread);
            stackPop(thread);
            Something *t = stackReserveFrame(NOTHINGNESS, 1, thread);
            *t = somethingObject(dico);
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end) {
                Something key;
                produce(consumeCoin(thread), thread, &key);
                Something sth;
                produce(consumeCoin(thread), thread, &sth);
                
                dictionarySet(t->object, key.object, sth, thread);
            }
            *destination = *t;
            stackPushReservedFrame(thread);
            stackPop(thread);
            return;
        }
        case 0x51: {
            Something *t = stackReserveFrame(NOTHINGNESS, 1, thread);
            *t = somethingObject(newObject(CL_LIST));
            
            EmojicodeCoin *end = thread->tokenStream + consumeCoin(thread);
            while (thread->tokenStream < end) {
                Something sth;
                produce(consumeCoin(thread), thread, &sth);
                listAppend(t->object, sth, thread);
            }
            
            *destination = *t;
            stackPushReservedFrame(thread);
            stackPop(thread);
            return;
        }
        case 0x52: {
            EmojicodeCoin stringCount = consumeCoin(thread);
            Something *t = stackReserveFrame(NOTHINGNESS, stringCount + 1, thread);
            
            EmojicodeInteger length = 0;
            
            for (EmojicodeCoin i = 0; i < stringCount; i++) {
                produce(consumeCoin(thread), thread, t + i);
                String *string = t[i].object->value;
                length += string->length;
            }
            
            stackPushReservedFrame(thread);
            
            Object *object = newObject(CL_STRING);

            *stackVariableDestination(stringCount, thread) = somethingObject(object);
            
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

            *destination = sm;
            return;
        }
        case 0x53: {
            Something start;
            produce(consumeCoin(thread), thread, &start);
            Something stop;
            produce(consumeCoin(thread), thread, &stop);
            Object *object = newObject(CL_RANGE);
            EmojicodeRange *range = object->value;
            range->start = start.raw;
            range->stop = stop.raw;
            rangeSetDefaultStep(range);
            *destination = somethingObject(object);
            return;
        }
        case 0x54: {
            Something start;
            produce(consumeCoin(thread), thread, &start);
            Something stop;
            produce(consumeCoin(thread), thread, &stop);
            Something step;
            produce(consumeCoin(thread), thread, &step);
            Object *object = newObject(CL_RANGE);
            EmojicodeRange *range = object->value;
            range->start = start.raw;
            range->stop = stop.raw;
            range->step = step.raw;
            if (range->step == 0) rangeSetDefaultStep(range);
            *destination = somethingObject(object);
            return;
        }
        case INS_BINARY_AND_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw & b.raw);
            return;
        }
        case INS_BINARY_OR_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw | b.raw);
            return;
        }
        case INS_BINARY_XOR_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw ^ b.raw);
            return;
        }
        case INS_BINARY_NOT_INTEGER: {
            Something a;
            produce(consumeCoin(thread), thread, &a);
            *destination = somethingInteger(~a.raw);
            return;
        }
        case INS_SHIFT_LEFT_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw << b.raw);
            return;
        }
        case INS_SHIFT_RIGHT_INTEGER: {
            Something a;
            Something b;
            produce(consumeCoin(thread), thread, &a);
            produce(consumeCoin(thread), thread, &b);
            *destination = somethingInteger(a.raw >> b.raw);
            return;
        }            //MARK: Flow Control
        case INS_RETURN:
            produce(consumeCoin(thread), thread, thread->returnDestination);
            thread->tokenStream = NULL;
            return;
        case INS_DISCARD_PRODUCTION:
            produce(consumeCoin(thread), thread, (Something *)otherHeap);
            return;
        case INS_REPEAT_WHILE: {
            EmojicodeCoin *beginPosition = thread->tokenStream;
            Something sth;
            while (produce(consumeCoin(thread), thread, &sth), unwrapBool(sth)) {
                if (runBlock(thread)) {
                    return;
                }
                thread->tokenStream = beginPosition;
            }
            passBlock(thread);
            return;
        }
        case INS_IF: { //MARK: If
            EmojicodeCoin length = consumeCoin(thread);
            EmojicodeCoin *ifEnd = thread->tokenStream + length;

            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            if (unwrapBool(sth)) {  // Main if
                if (runBlock(thread)) {
                    return;
                }
                thread->tokenStream = ifEnd;
            }
            else {
                passBlock(thread);
                
                while (thread->tokenStream < ifEnd && nextCoin(thread) == 0x63) {  // All else ifs
                    consumeCoin(thread);
                    
                    produce(consumeCoin(thread), thread, &sth);
                    if (unwrapBool(sth)) {
                        if (runBlock(thread)) {
                            return;
                        }
                        thread->tokenStream = ifEnd;
                        return;
                    }
                    else {
                        passBlock(thread);
                    }
                }
                
                if (thread->tokenStream < ifEnd) {  // Else block
                    if (runBlock(thread)) {
                        return;
                    }
                }
            }
            return;
        }
        case INS_FOREACH: {
            //The destination variable
            EmojicodeCoin variable = consumeCoin(thread);
            
            Something iteratee;
            produce(consumeCoin(thread), thread, &iteratee);

            Something iterator;
            performFunction(iteratee.object->class->protocolsTable[1 - iteratee.object->class->protocolsOffset][0],
                            iteratee, thread, &iterator);
            EmojicodeCoin enumeratorVindex = consumeCoin(thread);
//            stackSetVariable(enumeratorVindex, iterator, thread);
            
            Function *nextMethod = iterator.object->class->protocolsTable[0][0];
            Function *moreComing = iterator.object->class->protocolsTable[0][1];
            
            EmojicodeCoin *begin = thread->tokenStream;

            Something more;
            while (performFunction(moreComing, stackGetVariable(enumeratorVindex, thread), thread, &more), unwrapBool(more)) {
                performFunction(nextMethod, stackGetVariable(enumeratorVindex, thread), thread,
                                stackVariableDestination(variable, thread));
                
                if (runBlock(thread)) {
                    return;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            
            return;
        }
        case 0x65: { //MARK: foreach for lists
            //The destination variable
            EmojicodeCoin variable = consumeCoin(thread);
            
            //Get the list
            Something losm;
            produce(consumeCoin(thread), thread, &losm);
            
            EmojicodeCoin listObjectVariable = consumeCoin(thread);
            *stackVariableDestination(listObjectVariable, thread) = losm;
            List *list = losm.object->value;
            
            EmojicodeCoin *begin = thread->tokenStream;
            
            for (size_t i = 0; i < (list = stackGetVariable(listObjectVariable, thread).object->value)->count; i++) {
                *stackVariableDestination(variable, thread) = listGet(list, i);
                unwrapOptional(stackGetVariable(variable, thread));
                
                if (runBlock(thread)) {
                    return;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            return;
        }
        case 0x66: {
            EmojicodeCoin variable = consumeCoin(thread);
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            EmojicodeRange range = *(EmojicodeRange *)sth.object->value;
            EmojicodeCoin *begin = thread->tokenStream;
            for (EmojicodeInteger i = range.start; i != range.stop; i += range.step) {
                *stackVariableDestination(variable, thread) = somethingInteger(i);
                
                if (runBlock(thread)) {
                    return;
                }
                thread->tokenStream = begin;
            }
            passBlock(thread);
            return;
        }
        case INS_EXECUTE_CALLABLE: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            Object *callable = sth.object;
            if (callable->class == CL_CAPTURED_FUNCTION_CALL) {
                CapturedFunctionCall *cmc = callable->value;
                performFunction(cmc->function, cmc->callee, thread, destination);
                return;
            }
            else {
                Closure *c = callable->value;
                stackPush(c->thisContext, c->variableCount, c->argumentCount, thread);
                
                loadCapture(c, thread);

                EmojicodeCoin *preCoinStream = thread->tokenStream;
                Something *preReturnDest = thread->returnDestination;

                thread->returnDestination = destination;
                thread->tokenStream = c->tokenStream;
                runFunctionPointerBlock(thread, c->coinCount);
                thread->tokenStream = preCoinStream;
                thread->returnDestination = preReturnDest;

                stackPop(thread);
                return;
            }
        }
        case INS_CLOSURE: {
            stackPush(stackGetThisContext(thread), 1, 0, thread);
            *stackVariableDestination(0, thread) = somethingObject(newObject(CL_CLOSURE));
            
            Object *co = stackGetVariable(0, thread).object;
            Closure *c = co->value;
            
            c->variableCount = consumeCoin(thread);
            c->coinCount = consumeCoin(thread);
            c->tokenStream = thread->tokenStream;
            thread->tokenStream += c->coinCount;
            
            EmojicodeCoin argumentCount = consumeCoin(thread);
            c->argumentCount = argumentCount;
            
            stackPop(thread);
            
            c->captureCount = consumeCoin(thread);
            EmojicodeInteger size = consumeCoin(thread);
            
            Object *capture = newArray(sizeof(Something) * size);
            c->capturedVariables = capture;
            Object *destinations = newArray(sizeof(CaptureInformation) * c->captureCount);
            c->capturesInformation = destinations;

            Something *t = capture->value;
            CaptureInformation *info = destinations->value;
            for (int i = 0; i < c->captureCount; i++) {
                EmojicodeInteger index = consumeCoin(thread);
                EmojicodeInteger size = consumeCoin(thread);
                info->destination = consumeCoin(thread);
                (info++)->size = size;
                memcpy(t, stackVariableDestination(index, thread), size * sizeof(Something));
                t += size;
            }
            
            if (argumentCount >> 16)
                c->thisContext = stackGetThisContext(thread);
            
            *destination = somethingObject(co);
            return;
        }
        case INS_CAPTURE_METHOD: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            stackPush(sth, 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = stackGetThisObject(thread)->class->methodsVtable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            *destination = somethingObject(cmco);
            return;
        }
        case INS_CAPTURE_TYPE_METHOD: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            stackPush(sth, 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = stackGetThisContext(thread).eclass->methodsVtable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            *destination = somethingObject(cmco);
            return;
        }
        case INS_CAPTURE_CONTEXTED_FUNCTION: {
            Something sth;
            produce(consumeCoin(thread), thread, &sth);
            stackPush(sth, 0, 0, thread);
            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = cmco->value;
            
            EmojicodeCoin vti = consumeCoin(thread);
            cmc->function = functionTable[vti];
            cmc->callee = stackGetThisContext(thread);
            stackPop(thread);
            *destination = somethingObject(cmco);
            return;
        }
    }
    error("Illegal bytecode instruction %X", coin);
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
    Something sth;
    performFunction(handler, NOTHINGNESS, mainThread, &sth);
    return (int)sth.raw;
}
