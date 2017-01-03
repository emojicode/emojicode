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
#include "../utf8.h"
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

//MARK: System

static void systemExit(Thread *thread, Something *destination) {
    EmojicodeInteger state = unwrapInteger(stackGetVariable(0, thread));
    exit((int)state);
}

static void systemGetEnv(Thread *thread, Something *destination) {
    char* variableName = stringToChar(stackGetVariable(0, thread).object->value);
    char* env = getenv(variableName);

    if (!env) {
        *destination = NOTHINGNESS;
        return;
    }

    free(variableName);
    *destination = somethingObject(stringFromChar(env));
}

static void systemCWD(Thread *thread, Something *destination) {
    char path[1050];
    getcwd(path, sizeof(path));

    *destination = somethingObject(stringFromChar(path));
}

static void systemTime(Thread *thread, Something *destination) {
    *destination = somethingInteger(time(NULL));
}

static void systemArgs(Thread *thread, Something *destination) {
    stackPush(NOTHINGNESS, 1, 0, thread);

    Object *listObject = newObject(CL_LIST);
    *stackVariableDestination(0, thread) = somethingObject(listObject);

    List *newList = listObject->value;
    newList->capacity = cliArgumentCount;
    Object *items = newArray(sizeof(Something) * cliArgumentCount);

    listObject = stackGetVariable(0, thread).object;

    ((List *)listObject->value)->items = items;

    for (int i = 0; i < cliArgumentCount; i++) {
        listAppend(listObject, somethingObject(stringFromChar(cliArguments[i])), thread);
    }

    stackPop(thread);
    *destination = somethingObject(listObject);
}

static void systemSystem(Thread *thread, Something *destination) {
    char *command = stringToChar(stackGetVariable(0, thread).object->value);
    FILE *f = popen(command, "r");
    free(command);

    if (!f) {
        *destination = NOTHINGNESS;
        return;
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
    *stackVariableDestination(0, thread) = somethingObject(so);
    String *string = so->value;
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = stackGetVariable(0, thread).object->value;
    string->characters = chars;

    u8_toucs(characters(string), len, buffer->value, bufferUsedSize);

    *destination = stackGetVariable(0, thread);
}

//MARK: Threads

void* threadStarter(void *threadv) {
    Thread *thread = threadv;
    Object *callable = stackGetThisObject(thread);
    stackPop(thread);
    executeCallableExtern(callable, NULL, thread, NULL);
    removeThread(thread);
    return NULL;
}

static void threadJoin(Thread *thread, Something *destination) {
    allowGC();
    pthread_join(*(pthread_t *)((Object *)stackGetThisObject(thread))->value, NULL);  // TODO: GC?!
    disallowGCAndPauseIfNeeded();
}

static void threadSleep(Thread *thread, Something *destination) {
    sleep((unsigned int)stackGetVariable(0, thread).raw);
}

static void threadSleepMicroseconds(Thread *thread, Something *destination) {
    usleep((unsigned int)stackGetVariable(0, thread).raw);
}

static void initThread(Thread *thread, Something *destination) {
    Thread *t = allocateThread();
    stackPush(stackGetVariable(0, thread), 0, 0, t);
    pthread_create((pthread_t *)((Object *)stackGetThisObject(thread))->value, NULL, threadStarter, t);
}

static void initMutex(Thread *thread, Something *destination) {
    pthread_mutex_init(((Object *)stackGetThisObject(thread))->value, NULL);
}

static void mutexLock(Thread *thread, Something *destination) {
    while (pthread_mutex_trylock(((Object *)stackGetThisObject(thread))->value) != 0) {
        //TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        //the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        //allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC(NULL);
        usleep(10);
    }
}

static void mutexUnlock(Thread *thread, Something *destination) {
    pthread_mutex_unlock(((Object *)stackGetThisObject(thread))->value);
}

static void mutexTryLock(Thread *thread, Something *destination) {
    *destination = pthread_mutex_trylock(((Object *)stackGetThisObject(thread))->value) == 0 ? EMOJICODE_TRUE :
                                                                                               EMOJICODE_FALSE;
}

//MARK: Error

Object* newError(const char *message, int code){
    Object *o = newObject(CL_ERROR);

    EmojicodeError* error = o->value;
    error->message = message;
    error->code = code;

    return o;
}

void initErrorBridge(Thread *thread, Something *destination) {
    EmojicodeError *error = stackGetThisObject(thread)->value;
    error->message = stringToChar(stackGetVariable(0, thread).object->value);
    error->code = unwrapInteger(stackGetVariable(1, thread));
}

static void errorGetMessage(Thread *thread, Something *destination) {
    EmojicodeError *error = stackGetThisObject(thread)->value;
    *destination = somethingObject(stringFromChar(error->message));
}

static void errorGetCode(Thread *thread, Something *destination) {
    EmojicodeError *error = stackGetThisObject(thread)->value;
    *destination = somethingInteger((EmojicodeInteger)error->code);
}

//MARK: Data

static void dataEqual(Thread *thread, Something *destination) {
    Data *d = stackGetThisObject(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;

    if(d->length != b->length){
        *destination = EMOJICODE_FALSE;
        return;
    }

    *destination = memcmp(d->bytes, b->bytes, d->length) == 0 ? EMOJICODE_TRUE : EMOJICODE_FALSE;
}

static void dataSize(Thread *thread, Something *destination) {
    Data *d = stackGetThisObject(thread)->value;
    *destination = somethingInteger((EmojicodeInteger)d->length);
}

static void dataMark(Object *o) {
    Data *d = o->value;
    if (d->bytesObject) {
        mark(&d->bytesObject);
        d->bytes = d->bytesObject->value;
    }
}

static void dataGetByte(Thread *thread, Something *destination) {
    Data *d = stackGetThisObject(thread)->value;

    EmojicodeInteger index = unwrapInteger(stackGetVariable(0, thread));
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index){
        *destination = NOTHINGNESS;
        return;
    }

    *destination = somethingInteger((unsigned char)d->bytes[index]);
}

static void dataToString(Thread *thread, Something *destination) {
    Data *data = stackGetThisObject(thread)->value;
    if (!u8_isvalid(data->bytes, data->length)) {
        *destination = NOTHINGNESS;
        return;
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
    *destination = somethingObject(sto);
}

static void dataSlice(Thread *thread, Something *destination) {
    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->value;
    Data *data = stackGetThisObject(thread)->value;

    EmojicodeInteger from = stackGetVariable(0, thread).raw;
    if (from >= data->length) {
        *destination = somethingObject(ooData);
        return;
    }

    EmojicodeInteger l = stackGetVariable(1, thread).raw;
    if (stackGetVariable(0, thread).raw + l > data->length) {
        l = data->length - stackGetVariable(0, thread).raw;
    }

    oData->bytesObject = data->bytesObject;
    oData->bytes = data->bytes + from;
    oData->length = l;
    *destination = somethingObject(ooData);
}

static void dataIndexOf(Thread *thread, Something *destination) {
    Data *data = stackGetThisObject(thread)->value;
    Data *search = stackGetVariable(0, thread).object->value;
    void *location = findBytesInBytes(data->bytes, data->length, search->bytes, search->length);
    if (!location) {
        *destination = NOTHINGNESS;
    }
    else {
        *destination = somethingInteger((Byte *)location - (Byte *)data->bytes);
    }
}

static void dataByAppendingData(Thread *thread, Something *destination) {
    Data *data = stackGetThisObject(thread)->value;
    Data *b = stackGetVariable(0, thread).object->value;

    size_t size = data->length + b->length;
    Object *newBytes = newArray(size);

    b = stackGetVariable(0, thread).object->value;
    data = stackGetThisObject(thread)->value;

    memcpy(newBytes->value, data->bytes, data->length);
    memcpy((Byte *)newBytes->value + data->length, b->bytes, b->length);

    *stackVariableDestination(0, thread) = somethingObject(newBytes);
    Object *ooData = newObject(CL_DATA);
    Data *oData = ooData->value;
    oData->bytesObject = stackGetVariable(0, thread).object;
    oData->bytes = oData->bytesObject->value;
    oData->length = size;
    *destination = somethingObject(ooData);
}

// MARK: Integer

void integerToString(Thread *thread, Something *destination) {
    EmojicodeInteger base = stackGetVariable(0, thread).raw;
    EmojicodeInteger n = stackGetThisContext(thread).raw, a = llabs(n);
    bool negative = n < 0;

    EmojicodeInteger d = negative ? 2 : 1;
    while (n /= base) d++;

    Object *co = newArray(d * sizeof(EmojicodeChar));
    *stackVariableDestination(0, thread) = somethingObject(co);

    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->value;
    string->length = d;
    string->characters = stackGetVariable(0, thread).object;

    EmojicodeChar *characters = characters(string) + d;
    do
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    while (a /= base);

    if (negative) characters[-1] = '-';

    *destination = somethingObject(stringObject);
}

static void integerRandom(Thread *thread, Something *destination) {
    *stackGetThisContext(thread).something = somethingInteger(secureRandomNumber(stackGetVariable(0, thread).raw, stackGetVariable(1, thread).raw));
}

#define abs(x) _Generic((x), long: labs, long long: llabs, default: abs)(x)

static void integerAbsolute(Thread *thread, Something *destination) {
    *destination = somethingInteger(abs(stackGetThisContext(thread).raw));
}

static void symbolToString(Thread *thread, Something *destination) {
    Object *co = newArray(sizeof(EmojicodeChar));
    stackPush(somethingObject(co), 0, 0, thread);
    Object *stringObject = newObject(CL_STRING);
    String *string = stringObject->value;
    string->length = 1;
    string->characters = stackGetThisObject(thread);
    stackPop(thread);
    ((EmojicodeChar *)string->characters->value)[0] = (EmojicodeChar)stackGetThisContext(thread).raw;
    *destination = somethingObject(stringObject);
}

static void symbolToInteger(Thread *thread, Something *destination) {
    *destination = stackGetThisContext(thread);
}

static void doubleToString(Thread *thread, Something *something) {
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
    *stackVariableDestination(0, thread) = somethingObject(co);
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
    *something = somethingObject(stringObject);
}

static void doubleSin(Thread *thread, Something *destination) {
    *destination = somethingDouble(sin(stackGetThisContext(thread).doubl));
}

static void doubleCos(Thread *thread, Something *destination) {
    *destination = somethingDouble(cos(stackGetThisContext(thread).doubl));
}

static void doubleTan(Thread *thread, Something *destination) {
    *destination = somethingDouble(tan(stackGetThisContext(thread).doubl));
}

static void doubleASin(Thread *thread, Something *destination) {
    *destination = somethingDouble(asin(stackGetThisContext(thread).doubl));
}

static void doubleACos(Thread *thread, Something *destination) {
    *destination = somethingDouble(acos(stackGetThisContext(thread).doubl));
}

static void doubleATan(Thread *thread, Something *destination) {
    *destination = somethingDouble(atan(stackGetThisContext(thread).doubl));
}

static void doublePow(Thread *thread, Something *destination) {
    *destination = somethingDouble(pow(stackGetThisContext(thread).doubl, stackGetVariable(0, thread).doubl));
}

static void doubleSqrt(Thread *thread, Something *destination) {
    *destination = somethingDouble(sqrt(stackGetThisContext(thread).doubl));
}

static void doubleRound(Thread *thread, Something *destination) {
    *destination = somethingInteger(round(stackGetThisContext(thread).doubl));
}

static void doubleCeil(Thread *thread, Something *destination) {
    *destination = somethingInteger(ceil(stackGetThisContext(thread).doubl));
}

static void doubleFloor(Thread *thread, Something *destination) {
    *destination = somethingInteger(floor(stackGetThisContext(thread).doubl));
}

static void doubleLog2(Thread *thread, Something *destination) {
    *destination = somethingDouble(log2(stackGetThisContext(thread).doubl));
}

static void doubleLn(Thread *thread, Something *destination) {
    *destination = somethingDouble(log(stackGetThisContext(thread).doubl));
}

static void doubleAbsolute(Thread *thread, Something *destination) {
    *destination = somethingDouble(fabs(stackGetThisContext(thread).doubl));
}

// MARK: Callable

static void closureMark(Object *o){
    Closure *c = o->value;
    if (isRealObject(c->thisContext)) {
        mark(&c->thisContext.object);
    }
    mark(&c->capturedVariables);

    Something *t = c->capturedVariables->value;
    CaptureInformation *infop = c->capturesInformation->value;
    for (int i = 0; i < c->captureCount; i++) {
        Something *s = t + (infop++)->destination;
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

FunctionFunctionPointer sLinkingTable[] = {
    NULL,
    //📇
    [1] = dataEqual,
    [2] = dataSize, //🐔
    [3] = dataGetByte, //🐽
    [4] = dataToString, //🔡
    [5] = dataSlice, //🔪
    [6] = dataIndexOf, //🔍
    [7] = dataByAppendingData, //📝
    //💈
    [8] = initThread,
    [9] = threadSleep, //⏳
    [10] = threadSleepMicroseconds, //⏲
    [11] = threadJoin, //🛂
    //🔐
    [12] = initMutex,
    [13] = mutexLock, //🔒
    [14] = mutexUnlock, //🔓
    [15] = mutexTryLock, //🔐
    //🚂
    [16] = integerToString, //🔡
    [17] = integerRandom, //🎰
    [18] = integerAbsolute, //🏧
    [19] = doubleSin, //📓
    [20] = doubleCos, //📕
    [21] = doubleTan, //📘
    [22] = doubleASin, //📔
    [23] = doubleACos, //📙
    [24] = doubleATan, //📗
    [25] = doublePow, //🏂
    [26] = doubleSqrt, //⛷
    [27] = doubleCeil, //🚴
    [28] = doubleFloor, //🚵
    [29] = doubleRound, //🏇
    [30] = doubleLog2, //🚣
    [31] = doubleLn, //🏄
    [32] = doubleToString, //🔡
    [33] = doubleAbsolute, //🏧
    //🔣
    [34] = symbolToString, //🔡
    [35] = symbolToInteger, //🚂
    //💻
    [36] = systemExit, //🚪
    [37] = systemGetEnv, //🌳
    [38] = systemCWD,
    [39] = systemTime, //🕰
    [40] = systemArgs, //🎞
    [41] = systemSystem, //🕴
    [45] = listAppendBridge, //bear
    [46] = listGetBridge, //🐽
    [47] = listRemoveBridge, //koala
    [48] = listInsertBridge, //monkey
    [49] = listCountBridge, //🐔
    [50] = listPopBridge, //panda
    [51] = listShuffleInPlaceBridge, //🐹
    [52] = listFromListBridge, //🐮
    [53] = listSort, //🦁
    [54] = listRemoveAllBridge, //🐗
    [55] = listSetBridge, //🐷
    [56] = listEnsureCapacityBridge, //🐴
    [57] = initListWithCapacity, //🐧
    [58] = initListEmptyBridge,
    [59] = stringPrintStdoutBrigde,
    [60] = stringEqualBridge, //🐔
    [61] = stringLengthBridge, //📝
    [62] = stringByAppendingSymbolBridge, //🐽
    [63] = stringSymbolAtBridge, //🔪
    [64] = stringSubstringBridge, //🔍
    [65] = stringIndexOf, //🔧
    [66] = stringTrimBridge, //🔫
    [67] = stringSplitByStringBridge, //📐
    [68] = stringUTF8LengthBridge, //💣
    [69] = stringSplitBySymbolBridge, //🎼
    [70] = stringBeginsWithBridge, //⛳️
    [71] = stringEndsWithBridge, //🎶
    [72] = stringToCharacterList, //📇
    [73] = stringToData, //📰
    [74] = stringJSON, //🚂
    [75] = stringToInteger, //🚀
    [76] = stringToDouble, //↔️
    [77] = stringCompareBridge, //📫
    [78] = stringToUppercase, //📪
    [79] = stringToLowercase,
    [80] = stringGetInput, //😮
    [81] = stringFromSymbolListBridge, //🎙
    [82] = stringFromStringList, //🍨
    [83] = errorGetMessage,
    [84] = errorGetCode,
    [85] = initErrorBridge,
    [86] = initDictionaryBridge,
    [87] = bridgeDictionaryGet, //🐽
    [88] = bridgeDictionaryRemove, //🐨
    [89] = bridgeDictionarySet, //🐷
    [90] = bridgeDictionaryKeys, //🐙
    [91] = bridgeDictionaryClear, //🐗
    [92] = bridgeDictionaryContains, //🐣
    [93] = bridgeDictionarySize, //🐔
};

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
