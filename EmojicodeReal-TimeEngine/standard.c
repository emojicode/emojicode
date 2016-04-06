//
//  ElementaryTypes.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <string.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>
#include <pthread.h>
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

static Something systemTime(Thread *thread) {
    return somethingInteger(time(NULL));
}

static Something systemArgs(Thread *thread) {
    stackPush(NULL, 1, 0, thread);
    
    Object *listObject = newObject(CL_LIST);
    stackSetVariable(0, somethingObject(listObject), thread);
    
    List *newList = listObject->value;
    newList->capacity = cliArgumentCount;
    Object *items = newArray(sizeof(Something) * cliArgumentCount);
    
    listObject = stackGetVariable(0, thread).object;
    
    ((List *)listObject->value)->items = items;
    
    for (int i = 0; i < cliArgumentCount; i++) {
        listAppend(listObject, somethingObject(stringFromChar(cliArguments[i])), thread);
    }
    
    stackPop(thread);
    return somethingObject(listObject);
}

static Something systemSystem(Thread *thread) {
    char *command = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *f = popen(command, "r");
    free(command);
    
    if (!f) {
        return NOTHINGNESS;
    }
    
    size_t bufferUsedSize = 0;
    int bufferSize = 50;
    Object *buffer = newArray(bufferSize);
    
    while (fgets((char *)buffer->value + bufferUsedSize, bufferSize - (int)bufferUsedSize, f) != NULL) {
        bufferUsedSize = strlen(buffer->value);
        
        if (bufferSize - bufferUsedSize < 2) {
            bufferSize *= 2;
            buffer = resizeArray(buffer, bufferSize);
        }
    }
    
    bufferUsedSize = strlen(buffer->value);
    
    EmojicodeInteger len = u8_strlen_l(buffer->value, bufferUsedSize);
    
    Object *so = newObject(CL_STRING);
    stackSetVariable(0, somethingObject(so), thread);
    String *string = so->value;
    string->length = len;
    
    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = stackGetVariable(0, thread).object->value;
    string->characters = chars;
    
    u8_toucs(characters(string), len, buffer->value, bufferUsedSize);
    
    return stackGetVariable(0, thread);
}

//MARK: Threads

void* threadStarter(void *threadv) {
    Thread *thread = threadv;
    Object *callable = stackGetThis(thread);
    stackPop(thread);
    executeCallableExtern(callable, NULL, thread);
    removeThread(thread);
    return NULL;
}

static Something threadJoin(Thread *thread) {
    allowGC();
    bool l = pthread_join(*(pthread_t *)((Object *)stackGetThis(thread))->value, NULL) == 0;
    disallowGCAndPauseIfNeeded();
    return l ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something threadSleep(Thread *thread){
    sleep((unsigned int)stackGetVariable(0, thread).raw);
    return NOTHINGNESS;
}

static void initThread(Thread *thread) {
    Thread *t = allocateThread();
    stackPush(stackGetVariable(0, thread).object, 0, 0, t);
    pthread_create((pthread_t *)((Object *)stackGetThis(thread))->value, NULL, threadStarter, t);
}

static void initMutex(Thread *thread) {
    pthread_mutex_init(((Object *)stackGetThis(thread))->value, NULL);
}

static Something mutexLock(Thread *thread) {
    while (pthread_mutex_trylock(((Object *)stackGetThis(thread))->value) != 0) {
        //TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        //the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        //allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC(NULL);
        usleep(10);
    }
    return NOTHINGNESS;
}

static Something mutexUnlock(Thread *thread) {
    pthread_mutex_unlock(((Object *)stackGetThis(thread))->value);
    return NOTHINGNESS;
}

static Something mutexTryLock(Thread *thread) {
    return pthread_mutex_trylock(((Object *)stackGetThis(thread))->value) == 0 ? EMOJICODE_TRUE : EMOJICODE_FALSE;
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

void rangeSetDefaultStep(EmojicodeRange *range) {
    range->step = range->start > range->stop ? -1 : 1;
}

static void initRangeStartStop(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
    rangeSetDefaultStep(range);
}

static void initRangeStartStopStep(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
    range->step = stackGetVariable(2, thread).raw;
    if (range->step == 0) rangeSetDefaultStep(range);
}

static Something rangeGet(Thread *thread) {
    EmojicodeRange *range = stackGetThis(thread)->value;
    EmojicodeInteger h = range->start + stackGetVariable(0, thread).raw * range->step;
    return (range->step > 0 ? range->start <= h && h < range->stop : range->stop < h && h <= range->start) ? somethingInteger(h) : NOTHINGNESS;
}

//MARK: Data

static Something dataEqual(Thread *thread) {
    Data *d = stackGetThis(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;
    
    if(d->length != b->length){
        return EMOJICODE_FALSE;
    }
    
    return memcmp(d->bytes, b->bytes, d->length) == 0 ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something dataSize(Thread *thread) {
    Data *d = stackGetThis(thread)->value;
    return somethingInteger((EmojicodeInteger)d->length);
}

static void dataMark(Object *o) {
    Data *d = o->value;
    if (d->bytesObject) {
        mark(&d->bytesObject);
        d->bytes = d->bytesObject->value;
    }
}

static Something dataGetByte(Thread *thread) {
    Data *d = stackGetThis(thread)->value;
    
    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index){
        return NOTHINGNESS;
    }
    
    return somethingInteger(d->bytes[index]);
}

//MARK: Math

static Something mathSin(Thread *thread) {
    return somethingDouble(sin(stackGetVariable(0, thread).doubl));
}

static Something mathCos(Thread *thread) {
    return somethingDouble(cos(stackGetVariable(0, thread).doubl));
}

static Something mathTan(Thread *thread) {
    return somethingDouble(tan(stackGetVariable(0, thread).doubl));
}

static Something mathASin(Thread *thread) {
    return somethingDouble(asin(stackGetVariable(0, thread).doubl));
}

static Something mathACos(Thread *thread) {
    return somethingDouble(acos(stackGetVariable(0, thread).doubl));
}

static Something mathATan(Thread *thread) {
    return somethingDouble(atan(stackGetVariable(0, thread).doubl));
}

static Something mathPow(Thread *thread) {
    return somethingDouble(pow(stackGetVariable(0, thread).doubl, stackGetVariable(1, thread).doubl));
}

static Something mathSqrt(Thread *thread) {
    return somethingDouble(sqrt(stackGetVariable(0, thread).doubl));
}

static Something mathRound(Thread *thread) {
    return somethingInteger(round(stackGetVariable(0, thread).doubl));
}

static Something mathCeil(Thread *thread) {
    return somethingInteger(ceil(stackGetVariable(0, thread).doubl));
}

static Something mathFloor(Thread *thread) {
    return somethingInteger(floor(stackGetVariable(0, thread).doubl));
}

static Something mathRandom(Thread *thread) {
    return somethingInteger(secureRandomNumber(stackGetVariable(0, thread).raw, stackGetVariable(1, thread).raw));
}

static Something mathLog2(Thread *thread) {
    return somethingDouble(log2(stackGetVariable(0, thread).doubl));
}

static Something mathLn(Thread *thread) {
    return somethingDouble(log(stackGetVariable(0, thread).doubl));
}


//MARK: Callable

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
                case 0x1F43D:
                    return dataGetByte;
            }
        case 0x1F36F:
            return dictionaryMethodForName(symbol);
        case 0x23E9:
            // case 0x1F43D: //pig nose
            return rangeGet;
        case 0x1f488: //💈
            return threadJoin;
        case 0x1f510: //🔐
            switch (symbol) {
                case 0x1f512: //🔒
                    return mutexLock;
                case 0x1f513: //🔓
                    return mutexUnlock;
                case 0x1f510: //🔐
                    return mutexTryLock;
            }
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
        case 0x1f488: //💈
            return initThread;
        case 0x1f510: //🔐
            return initMutex;
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
    switch (cl) {
        case 0x1F4BB: //💻
            switch (symbol) {
                case 0x1F6AA:
                    return systemExit;
                case 0x1F333:
                    return systemGetEnv;
                case 0x1F30D:
                    return systemCWD;
                case 0x1f570: //🕰
                    return systemTime;
                case 0x1f39e: //🎞
                    return systemArgs;
                case 0x1f574: //🕴
                    return systemSystem;
            }
            break;
        case 0x1F684: //🚄
            switch (symbol) {
                case 0x1f4d3: //📓
                    return mathSin;
                case 0x1f4d8: //📘
                    return mathTan;
                case 0x1f4d5: //📕
                    return mathCos;
                case 0x1f4d4: //📔
                    return mathASin;
                case 0x1f4d9: //📙
                    return mathACos;
                case 0x1f4d7: //📗
                    return mathATan;
                case 0x1f3c2: //🏂
                    return mathPow;
                case 0x26f7: //⛷
                    return mathSqrt;
                case 0x1f6b4: //🚴
                    return mathCeil;
                case 0x1f6b5: //🚵
                    return mathFloor;
                case 0x1f3c7: //🏇
                    return mathRound;
                case 0x1f3b0: //🎰
                    return mathRandom;
                case 0x1f6a3: //🚣
                    return mathLog2;
                case 0x1f3c4: //🏄
                    return mathLn;
            }
        case 0x1f488: //💈
            //case 0x23f3: //⏳
            return threadSleep;
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
        case 0x1f488: //💈
            return sizeof(pthread_t);
        case 0x1f510: //🔐
            return sizeof(pthread_mutex_t);
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
        case 0x1F4C7:
            return dataMark;
    }
    return NULL;
}

Deinitializer deinitializerPointerForClass(EmojicodeChar cl){
    return NULL;
}
