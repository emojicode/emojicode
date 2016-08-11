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
#include "algorithms.h"

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
    stackPush(NOTHINGNESS, 1, 0, thread);
    
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
    Object *callable = stackGetThisObject(thread);
    stackPop(thread);
    executeCallableExtern(callable, NULL, thread);
    removeThread(thread);
    return NULL;
}

static Something threadJoin(Thread *thread) {
    allowGC();
    bool l = pthread_join(*(pthread_t *)((Object *)stackGetThisObject(thread))->value, NULL) == 0;
    disallowGCAndPauseIfNeeded();
    return l ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something threadSleep(Thread *thread){
    sleep((unsigned int)stackGetVariable(0, thread).raw);
    return NOTHINGNESS;
}

static Something threadSleepMicroseconds(Thread *thread){
    usleep((unsigned int)stackGetVariable(0, thread).raw);
    return NOTHINGNESS;
}

static void initThread(Thread *thread) {
    Thread *t = allocateThread();
    stackPush(stackGetVariable(0, thread), 0, 0, t);
    pthread_create((pthread_t *)((Object *)stackGetThisObject(thread))->value, NULL, threadStarter, t);
}

static void initMutex(Thread *thread) {
    pthread_mutex_init(((Object *)stackGetThisObject(thread))->value, NULL);
}

static Something mutexLock(Thread *thread) {
    while (pthread_mutex_trylock(((Object *)stackGetThisObject(thread))->value) != 0) {
        //TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        //the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        //allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC(NULL);
        usleep(10);
    }
    return NOTHINGNESS;
}

static Something mutexUnlock(Thread *thread) {
    pthread_mutex_unlock(((Object *)stackGetThisObject(thread))->value);
    return NOTHINGNESS;
}

static Something mutexTryLock(Thread *thread) {
    return pthread_mutex_trylock(((Object *)stackGetThisObject(thread))->value) == 0 ? EMOJICODE_TRUE : EMOJICODE_FALSE;
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
    EmojicodeError *error = stackGetThisObject(thread)->value;
    error->message = stringToChar(stackGetVariable(0, thread).object->value);
    error->code = unwrapInteger(stackGetVariable(1, thread));
}

static Something errorGetMessage(Thread *thread){
    EmojicodeError *error = stackGetThisObject(thread)->value;
    return somethingObject(stringFromChar(error->message));
}

static Something errorGetCode(Thread *thread){
    EmojicodeError *error = stackGetThisObject(thread)->value;
    return somethingInteger((EmojicodeInteger)error->code);
}

//MARK: Range

void rangeSetDefaultStep(EmojicodeRange *range) {
    range->step = range->start > range->stop ? -1 : 1;
}

static void initRangeStartStop(Thread *thread) {
    EmojicodeRange *range = stackGetThisObject(thread)->value;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
    rangeSetDefaultStep(range);
}

static void initRangeStartStopStep(Thread *thread) {
    EmojicodeRange *range = stackGetThisObject(thread)->value;
    range->start = stackGetVariable(0, thread).raw;
    range->stop = stackGetVariable(1, thread).raw;
    range->step = stackGetVariable(2, thread).raw;
    if (range->step == 0) rangeSetDefaultStep(range);
}

static Something rangeGet(Thread *thread) {
    EmojicodeRange *range = stackGetThisObject(thread)->value;
    EmojicodeInteger h = range->start + stackGetVariable(0, thread).raw * range->step;
    return (range->step > 0 ? range->start <= h && h < range->stop : range->stop < h && h <= range->start) ? somethingInteger(h) : NOTHINGNESS;
}

//MARK: Data

static Something dataEqual(Thread *thread) {
    Data *d = stackGetThisObject(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;
    
    if(d->length != b->length){
        return EMOJICODE_FALSE;
    }
    
    return memcmp(d->bytes, b->bytes, d->length) == 0 ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static Something dataSize(Thread *thread) {
    Data *d = stackGetThisObject(thread)->value;
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
    Data *d = stackGetThisObject(thread)->value;
    
    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index){
        return NOTHINGNESS;
    }
    
    return somethingInteger((unsigned char)d->bytes[index]);
}

static Something dataToString(Thread *thread) {
    Data *data = stackGetThisObject(thread)->value;
    if (!u8_isvalid(data->bytes, data->length)) {
        return NOTHINGNESS;
    }
    
    EmojicodeInteger len = u8_strlen_l(data->bytes, data->length);
    Object *characters = newArray(len * sizeof(EmojicodeChar));
    
    stackPush(somethingObject(characters), 0, 0, thread);
    Object *sto = newObject(CL_STRING);
    String *string = sto->value;
    string->length = len;
    string->characters = stackGetThisObject(thread);
    stackPop(thread);
    u8_toucs(characters(string), len, data->bytes, data->length);
    return somethingObject(sto);
}

static Something dataSlice(Thread *thread) {
    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->value;
    Data *data = stackGetThisObject(thread)->value;
    
    EmojicodeInteger from = stackGetVariable(0, thread).raw;
    if (from >= data->length) {
        return somethingObject(ooData);
    }
    
    EmojicodeInteger l = stackGetVariable(1, thread).raw;
    if (stackGetVariable(0, thread).raw + l > data->length) {
        l = data->length - stackGetVariable(0, thread).raw;
    }
    
    oData->bytesObject = data->bytesObject;
    oData->bytes = data->bytes + from;
    oData->length = l;
    return somethingObject(ooData);
}

static Something dataIndexOf(Thread *thread) {
    Data *data = stackGetThisObject(thread)->value;
    Data *search = stackGetVariable(0, thread).object->value;
    void *location = findBytesInBytes(data->bytes, data->length, search->bytes, search->length);
    if (!location) {
        return NOTHINGNESS;
    }
    else {
        return somethingInteger((Byte *)location - (Byte *)data->bytes);
    }
}

static Something dataByAppendingData(Thread *thread) {
    Data *data = stackGetThisObject(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;
    
    size_t size = data->length + b->length;
    Object *newBytes = newArray(size);
    
    b = stackGetVariable(0, thread).object->value;
    data = stackGetThisObject(thread)->value;
    
    memcpy(newBytes->value, data->bytes, data->length);
    memcpy((Byte *)newBytes->value + data->length, b->bytes, b->length);
    
    stackSetVariable(0, somethingObject(newBytes), thread);
    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->value;
    oData->bytesObject = stackGetVariable(0, thread).object;
    oData->bytes = oData->bytesObject->value;
    oData->length = size;
    return somethingObject(ooData);
}

// MARK: Integer

Something integerToString(Thread *thread) {
    EmojicodeInteger base = stackGetVariable(0, thread).raw;
    EmojicodeInteger n = stackGetThisContext(thread).raw, a = llabs(n);
    bool negative = n < 0;
    
    EmojicodeInteger d = negative ? 2 : 1;
    while (n /= base) d++;
    
    Object *co = newArray(d * sizeof(EmojicodeChar));
    stackSetVariable(0, somethingObject(co), thread);
    
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->value;
    string->length = d;
    string->characters = stackGetVariable(0, thread).object;
    
    EmojicodeChar *characters = characters(string) + d;
    do
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    while (a /= base);
    
    if (negative) characters[-1] = '-';
    
    return somethingObject(stringObject);
}

static Something integerRandom(Thread *thread) {
    return somethingInteger(secureRandomNumber(stackGetVariable(0, thread).raw, stackGetVariable(1, thread).raw));
}

#define abs(x) _Generic((x), long: labs, long long: llabs, default: abs)(x)

static Something integerAbsolute(Thread *thread) {
    return somethingInteger(abs(stackGetThisContext(thread).raw));
}

static Something symbolToString(Thread *thread){
    Object *co = newArray(sizeof(EmojicodeChar));
    stackPush(somethingObject(co), 0, 0, thread);
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->value;
    string->length = 1;
    string->characters = stackGetThisObject(thread);
    stackPop(thread);
    ((EmojicodeChar *)string->characters->value)[0] = (EmojicodeChar)stackGetThisContext(thread).raw;
    return somethingObject(stringObject);
}

static Something symbolToInteger(Thread *thread) {
    return stackGetThisContext(thread);
}

static Something doubleToString(Thread *thread) {
    EmojicodeInteger precision = stackGetVariable(0, thread).raw;
    double d = stackGetThisContext(thread).doubl;
    double absD = fabs(d);
    
    bool negative = d < 0;
    
    EmojicodeInteger length = negative ? 1 : 0;
    if (precision != 0) {
        length++;
    }
    length += precision;
    EmojicodeInteger iLength = 1;
    for (size_t i = 1; pow(10, i) < absD; i++) {
        iLength++;
    }
    length += iLength;
    
    Object *co = newArray(length * sizeof(EmojicodeChar));
    stackSetVariable(0, somethingObject(co), thread);
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->value;
    string->length = length;
    string->characters = stackGetVariable(0, thread).object;
    
    EmojicodeChar *characters = characters(string) + length;
    
    for (size_t i = precision; i > 0; i--) {
        *--characters =  (unsigned char) (fmod(absD * pow(10, i), 10.0)) % 10 + '0';
    }
    
    if (precision != 0) {
        *--characters = '.';
    }
    
    for (size_t i = 0; i < iLength; i++) {
        *--characters =  (unsigned char) (fmod(absD / pow(10, i), 10.0)) % 10 + '0';
    }
    
    if (negative) characters[-1] = '-';
    return somethingObject(stringObject);
}

static Something doubleSin(Thread *thread) {
    return somethingDouble(sin(stackGetThisContext(thread).doubl));
}

static Something doubleCos(Thread *thread) {
    return somethingDouble(cos(stackGetThisContext(thread).doubl));
}

static Something doubleTan(Thread *thread) {
    return somethingDouble(tan(stackGetThisContext(thread).doubl));
}

static Something doubleASin(Thread *thread) {
    return somethingDouble(asin(stackGetThisContext(thread).doubl));
}

static Something doubleACos(Thread *thread) {
    return somethingDouble(acos(stackGetThisContext(thread).doubl));
}

static Something doubleATan(Thread *thread) {
    return somethingDouble(atan(stackGetThisContext(thread).doubl));
}

static Something doublePow(Thread *thread) {
    return somethingDouble(pow(stackGetThisContext(thread).doubl, stackGetVariable(0, thread).doubl));
}

static Something doubleSqrt(Thread *thread) {
    return somethingDouble(sqrt(stackGetThisContext(thread).doubl));
}

static Something doubleRound(Thread *thread) {
    return somethingInteger(round(stackGetThisContext(thread).doubl));
}

static Something doubleCeil(Thread *thread) {
    return somethingInteger(ceil(stackGetThisContext(thread).doubl));
}

static Something doubleFloor(Thread *thread) {
    return somethingInteger(floor(stackGetThisContext(thread).doubl));
}

static Something doubleLog2(Thread *thread) {
    return somethingDouble(log2(stackGetThisContext(thread).doubl));
}

static Something doubleLn(Thread *thread) {
    return somethingDouble(log(stackGetThisContext(thread).doubl));
}

static Something doubleAbsolute(Thread *thread) {
    return somethingDouble(fabs(stackGetThisContext(thread).doubl));
}

// MARK: Callable

static void closureMark(Object *o){
    Closure *c = o->value;
    if (isRealObject(c->thisContext)) {
        mark(&c->thisContext.object);
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
    CapturedFunctionCall *c = o->value;
    if (isRealObject(c->callee)) {
        mark(&c->callee.object);
    }
}

FunctionFunctionPointer integerMethodForName(EmojicodeChar name);
FunctionFunctionPointer numberMethodForName(EmojicodeChar name);

FunctionFunctionPointer handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol, MethodType type) {
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
        case 0x1f4c7: //📇
            switch (symbol) {
                case 0x1F61B:
                    return dataEqual;
                case 0x1f414: //🐔
                    return dataSize;
                case 0x1f43d: //🐽
                    return dataGetByte;
                case 0x1f521: //🔡
                    return dataToString;
                case 0x1f52a: //🔪
                    return dataSlice;
                case 0x1f50d: //🔍
                    return dataIndexOf;
                case 0x1f4dd: //📝
                    return dataByAppendingData;
            }
        case 0x1F36F:
            return dictionaryMethodForName(symbol);
        case 0x23E9:
            // case 0x1F43D: //pig nose
            return rangeGet;
        case 0x1f488: //💈
            switch (symbol) {
                case 0x23f3: //⏳
                    return threadSleep;
                case 0x23f2: //⏲
                    return threadSleepMicroseconds;
                case 0x1f6c2: //🛂
                    return threadJoin;
            }
        case 0x1f510: //🔐
            switch (symbol) {
                case 0x1f512: //🔒
                    return mutexLock;
                case 0x1f513: //🔓
                    return mutexUnlock;
                case 0x1f510: //🔐
                    return mutexTryLock;
            }
        case 0x1F682: //🚂
            switch (symbol) {
                case 0x1f521: //🔡
                    return integerToString;
                case 0x1f3b0: //🎰
                    return integerRandom;
                case 0x1f3e7: //🏧
                    return integerAbsolute;
            }
        case 0x1F680:
            switch (symbol) {
                case 0x1f4d3: //📓
                    return doubleSin;
                case 0x1f4d8: //📘
                    return doubleTan;
                case 0x1f4d5: //📕
                    return doubleCos;
                case 0x1f4d4: //📔
                    return doubleASin;
                case 0x1f4d9: //📙
                    return doubleACos;
                case 0x1f4d7: //📗
                    return doubleATan;
                case 0x1f3c2: //🏂
                    return doublePow;
                case 0x26f7: //⛷
                    return doubleSqrt;
                case 0x1f6b4: //🚴
                    return doubleCeil;
                case 0x1f6b5: //🚵
                    return doubleFloor;
                case 0x1f3c7: //🏇
                    return doubleRound;
                case 0x1f6a3: //🚣
                    return doubleLog2;
                case 0x1f3c4: //🏄
                    return doubleLn;
                case 0x1f521: //🔡
                    return doubleToString;
                case 0x1f3e7: //🏧
                    return doubleAbsolute;
            }
        case 0x1f523: //🔣
            switch (symbol) {
                case 0x1f521: //🔡
                    return symbolToString;
                case 0x1f682: //🚂
                    return symbolToInteger;
            }
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
    }
    return NULL;
}

InitializerFunctionFunctionPointer handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol) {
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
            return sizeof(CapturedFunctionCall);
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
