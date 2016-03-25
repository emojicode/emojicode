//
//  ElementaryTypes.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "Emojicode.h"
#include "EmojicodeString.h"
#include "EmojicodeList.h"
#include "EmojicodeString.h"
#include "EmojicodeDictionary.h"
#include "utf8.h"

EmojicodeInteger secureRandomNumber(EmojicodeInteger min, EmojicodeInteger max){
    uint_fast64_t z;
    
#if defined __FreeBSD__ || defined __APPLE__ || defined __OpenBSD__ || defined __NetBSD__
    arc4random_buf(&z, sizeof(z));
#else
    FILE *fp = fopen("/dev/urandom", "r");
    fread(&z, 1, sizeof(z), fp);
    fclose(fp);
#endif
    
    return (z % (max - min + 1)) + min;
}

//MARK: Object

void objectNewBridge(Thread *thread){}

//MARK: System

static Something systemExit(Thread *thread){
    EmojicodeInteger state = unwrapInteger(stackGetVariable(0, thread));
    exit((int)state);
    return NOTHINGNESS;
}

static Something systemGetEnv(Thread *thread){
    char* variableName = stringToChar(stackGetVariable(0, thread).object->value);
    char* env = getenv(variableName);
    
    if(!env)
        return NOTHINGNESS;
    
    free(variableName);
    return somethingObject(stringFromChar(env));
}

static Something systemCWD(Thread *thread){
    char path[1050];
    getcwd(path, sizeof(path));
    
    return somethingObject(stringFromChar(path));
}

static Something sleepThread(Thread *thread){
    sleep((unsigned int)stackGetVariable(0, thread).raw);
    return NOTHINGNESS;
}

//MARK: Error

Object* newError(const char *message, int code){
    Object *o = newObject(CL_ERROR);
    
    EmojicodeError* error = o->value;
    error->message = message;
    error->code = code;
    
    return o;
}

void newErrorBridge(Thread *thread){
    EmojicodeError *error = stackGetThis(thread)->value;
    error->message = stringToChar(stackGetVariable(0, thread).object->value);
    error->code = unwrapInteger(stackGetVariable(1, thread));
}

static Something errorGetMessage(Thread *thread){
    EmojicodeError *error = stackGetThis(thread)->value;
    return somethingObject(stringFromChar(error->message));
}

static Something errorGetCode(Thread *thread){
    EmojicodeError *error = stackGetThis(thread)->value;
    return somethingInteger((EmojicodeInteger)error->code);
}

//MARK: Range

static void initRangeStartStop(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    range->step = 1;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
}

static void initRangeStartStopStep(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
    range->step = stackGetVariable(2, thread).raw;
}

static Something rangeGet(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    EmojicodeInteger h = range->start + stackGetVariable(0, thread).raw * range->step;
    return range->start <= h && h < range->stop ? somethingInteger(h) : NOTHINGNESS;
}

//MARK: Data

static Something dataEqual(Thread *thread){
    Data *d = stackGetThis(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;
    
    if(d->length != b->length){
        return EMOJICODE_FALSE;
    }
    
    for(size_t i = 0; i < d->length; i++){
        if(d->bytes[i] != b->bytes[i])
            return EMOJICODE_FALSE;
    }
    
    return EMOJICODE_TRUE;
}

static Something dataSize(Thread *thread){
    Data *d = stackGetThis(thread)->value;
    return somethingInteger((EmojicodeInteger)d->length);
}

static void closureMark(Object *o){
    Closure *c = o->value;
    if (isPossibleObjectPointer(c->this)) {
        mark((Object **)&c->this);
    }
    mark(&c->capturedVariables);
    
    Something *t = c->capturedVariables->value;
    for (uint8_t i = 0; i < c->capturedVariablesCount; i++) {
        Something *s = t + c->argumentCount + i;
        if (isRealObject(*s)) {
            mark(&s->object);
        }
    }
}

static void capturedMethodMark(Object *o){
    CapturedMethodCall *c = o->value;
    mark(&c->object);
}

MethodHandler integerMethodForName(EmojicodeChar name);
MethodHandler numberMethodForName(EmojicodeChar name);

MethodHandler handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol) {
    switch (cl) {
        case 0x1F521: //String
            return stringMethodForName(symbol);
        case 0x1F368: //List
            return listMethodForName(symbol);
        case 0x1F6A8: //Error
            switch (symbol) {
                case 0x1F624:
                    return errorGetMessage;
                case 0x1F634:
                    return errorGetCode;
            }
        case 0x1F4C7:
            switch (symbol) {
                case 0x1F61B:
                    return dataEqual;
                case 0x1F4CF: //📏
                    return dataSize;
            }
        case 0x1F36F:
            return dictionaryMethodForName(symbol);
        case 0x23E9:
            // case 0x1F43D: //pig nose
            return rangeGet;
    }
    return NULL;
}

InitializerHandler handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol){
    switch (cl) {
        case 0x1F535: //Object has a single initializer
            return objectNewBridge;
        case 0x1F368: //List
            return listInitializerForName(symbol);
        case 0x1F521: //String
            return stringInitializerForName(symbol);
        case 0x1F6A8: //Error’s only initializer 0x1F62E
            return newErrorBridge;
        case 0x1F36F: //Only dictionary contstructor 0x1F438
            return bridgeDictionaryInit;
        case 0x23E9:
            switch (symbol) {
                case 0x23E9:
                    return initRangeStartStop;
                case 0x23ED:
                    return initRangeStartStopStep;
            }
    }
    return NULL;
}

ClassMethodHandler handlerPointerForClassMethod(EmojicodeChar cl, EmojicodeChar symbol){
    switch (symbol) {
        case 0x1F6AA:
            return systemExit;
        case 0x1F333:
            return systemGetEnv;
        case 0x1F30D:
            return systemCWD;
        case 0x1F570:
            return sleepThread;
    }
    return NULL;
}

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1F521:
            return sizeof(String);
        case 0x1F368:
            return sizeof(List);
        case 0x1F36F:
            return sizeof(EmojicodeDictionary);
        case 0x1F4C7:
            return sizeof(Data);
        case 0x1F347:
            return sizeof(Closure);
        case 0x1F336:
            return sizeof(CapturedMethodCall);
        case 0x23E9:
            return sizeof(EmojicodeRange);
    }
    return 0;
}

Marker markerPointerForClass(EmojicodeChar cl){
    switch (cl) {
        case 0x1F368: //List
            return listMark;
        case 0x1F36F: //Dictionary
            return dictionaryMark;
        case 0x1F521:
            return stringMark;
        case 0x1F347:
            return closureMark;
        case 0x1F336:
            return capturedMethodMark;
    }
    return NULL;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl){
    switch (cl) {
//        case 0x1F4C7:
//            return releaseData;
    }
    return NULL;
}
