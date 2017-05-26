//
//  ElementaryTypes.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "standard.h"
#include "../utf8.h"
#include "Dictionary.h"
#include "Engine.hpp"
#include "List.h"
#include "String.h"
#include "String.h"
#include "Thread.hpp"
#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include <random>
#include <thread>
#include <unistd.h>

namespace Emojicode {

static void systemExit(Thread *thread) {
    EmojicodeInteger state = thread->getVariable(0).raw;
    exit((int)state);
}

static void systemGetEnv(Thread *thread) {
    char *env = getenv(stringToCString(thread->getVariable(0).object));

    if (!env) {
        thread->returnNothingnessFromFunction();
        return;
    }

    thread->returnOEValueFromFunction(stringFromChar(env));
}

static void systemCWD(Thread *thread) {
    char path[1050];
    getcwd(path, sizeof(path));
    thread->returnFromFunction(stringFromChar(path));
}

static void systemTime(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(time(nullptr)));
}

static void systemArgs(Thread *thread) {
    Object *const &listObject = thread->retain(newObject(CL_LIST));

    auto *newList = listObject->val<List>();
    newList->capacity = cliArgumentCount;
    Object *items = newArray(sizeof(Value) * cliArgumentCount);

    listObject->val<List>()->items = items;

    for (int i = 0; i < cliArgumentCount; i++) {
        listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringFromChar(cliArguments[i]));
    }

    thread->release(1);
    thread->returnFromFunction(listObject);
}

static void systemSystem(Thread *thread) {
    FILE *f = popen(stringToCString(thread->getVariable(0).object), "r");

    if (f == nullptr) {
        thread->returnNothingnessFromFunction();
        return;
    }

    size_t bufferUsedSize = 0;
    int bufferSize = 50;
    Object *buffer = newArray(bufferSize);

    while (fgets(buffer->val<char>() + bufferUsedSize, bufferSize - (int)bufferUsedSize, f) != nullptr) {
        bufferUsedSize = strlen(buffer->val<char>());

        if (bufferSize - bufferUsedSize < 2) {
            bufferSize *= 2;
            buffer = resizeArray(buffer, bufferSize);
        }
    }

    bufferUsedSize = strlen(buffer->val<char>());

    EmojicodeInteger len = u8_strlen_l(buffer->val<char>(), bufferUsedSize);

    Object *const &so = thread->retain(newObject(CL_STRING));
    auto *string = so->val<String>();
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = so->val<String>();
    string->charactersObject = chars;

    u8_toucs(string->characters(), len, buffer->val<char>(), bufferUsedSize);
    thread->release(1);
    thread->returnFromFunction(so);
}

//MARK: Threads

static void threadJoin(Thread *thread) {
    allowGC();
    auto cthread = thread->getThisObject()->val<std::thread>();
    cthread->join();  // TODO: GC?!
    disallowGCAndPauseIfNeeded();
    thread->returnFromFunction();
}

static void threadSleepMicroseconds(Thread *thread) {
    std::this_thread::sleep_for(std::chrono::microseconds(thread->getVariable(0).raw));
    thread->returnFromFunction();
}

void threadStart(Object *callable) {
    Thread thread = Thread();
    executeCallableExtern(callable, nullptr, 0, &thread, nullptr);
}

static void initThread(Thread *thread) {
    *thread->getThisObject()->val<std::thread>() = std::thread(threadStart, thread->getVariable(0).object);
    thread->returnFromFunction(thread->getThisContext());
}

static void initMutex(Thread *thread) {
    pthread_mutex_init(thread->getThisObject()->val<pthread_mutex_t>(), nullptr);
    thread->returnFromFunction(thread->getThisContext());
}

static void mutexLock(Thread *thread) {
    while (pthread_mutex_trylock(thread->getThisObject()->val<pthread_mutex_t>()) != 0) {
        // TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        // the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        // allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC();
        usleep(10);
    }
    thread->returnFromFunction();
}

static void mutexUnlock(Thread *thread) {
    pthread_mutex_unlock(thread->getThisObject()->val<pthread_mutex_t>());
    thread->returnFromFunction();
}

static void mutexTryLock(Thread *thread) {
    thread->returnFromFunction(pthread_mutex_trylock(thread->getThisObject()->val<pthread_mutex_t>()) == 0);
}

// MARK: Data

static void dataEqual(Thread *thread) {
    auto *d = thread->getThisObject()->val<Data>();
    auto *b = thread->getVariable(0).object->val<Data>();

    if (d->length != b->length) {
        thread->returnFromFunction(false);
        return;
    }

    thread->returnFromFunction(memcmp(d->bytes, b->bytes, d->length) == 0);
}

static void dataSize(Thread *thread) {
    thread->returnFromFunction(thread->getThisObject()->val<Data>()->length);
}

static void dataMark(Object *o) {
    auto *d = o->val<Data>();
    if (d->bytesObject) {
        mark(&d->bytesObject);
        d->bytes = d->bytesObject->val<char>();
    }
}

static void dataGetByte(Thread *thread) {
    auto *d = thread->getThisObject()->val<Data>();

    EmojicodeInteger index = thread->getVariable(0).raw;
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index) {
        thread->returnNothingnessFromFunction();
        return;
    }

    thread->returnOEValueFromFunction(EmojicodeInteger(d->bytes[index]));
}

static void dataToString(Thread *thread) {
    auto *data = thread->getThisObject()->val<Data>();
    if (u8_isvalid(data->bytes, data->length) == 0) {
        thread->returnNothingnessFromFunction();
        return;
    }

    EmojicodeInteger len = u8_strlen_l(data->bytes, data->length);
    Object *const &characters = thread->retain(newArray(len * sizeof(EmojicodeChar)));

    Object *sto = newObject(CL_STRING);
    auto *string = sto->val<String>();
    string->length = len;
    string->charactersObject = characters;
    thread->release(1);
    u8_toucs(string->characters(), len, data->bytes, data->length);
    thread->returnOEValueFromFunction(sto);
}

static void dataSlice(Thread *thread) {
    Object *ooData = newObject(CL_DATA);
    auto *oData = ooData->val<Data>();
    auto *data = thread->getThisObject()->val<Data>();

    EmojicodeInteger from = thread->getVariable(0).raw;
    if (from >= data->length) {
        thread->returnFromFunction(ooData);
        return;
    }

    EmojicodeInteger l = thread->getVariable(1).raw;
    if (thread->getVariable(0).raw + l > data->length) {
        l = data->length - thread->getVariable(0).raw;
    }

    oData->bytesObject = data->bytesObject;
    oData->bytes = data->bytes + from;
    oData->length = l;
    thread->returnFromFunction(ooData);
}

static void dataIndexOf(Thread *thread) {
    auto *data = thread->getThisObject()->val<Data>();
    auto *search = thread->getVariable(0).object->val<Data>();
    auto last = data->bytes + data->length;
    const void *location = std::search(data->bytes, last, search->bytes, search->bytes + search->length);
    if (location == last) {
        thread->returnNothingnessFromFunction();
    }
    else {
        thread->returnOEValueFromFunction(EmojicodeInteger((Byte *)location - (Byte *)data->bytes));
    }
}

static void dataByAppendingData(Thread *thread) {
    auto *data = thread->getThisObject()->val<Data>();
    auto *b = thread->getVariable(0).object->val<Data>();

    size_t size = data->length + b->length;
    Object *const &newBytes = thread->retain(newArray(size));

    b = thread->getVariable(0).object->val<Data>();
    data = thread->getThisObject()->val<Data>();

    std::memcpy(newBytes->val<char>(), data->bytes, data->length);
    std::memcpy(newBytes->val<char>() + data->length, b->bytes, b->length);

    Object *ooData = newObject(CL_DATA);
    auto *oData = ooData->val<Data>();
    oData->bytesObject = newBytes;
    oData->bytes = oData->bytesObject->val<char>();
    oData->length = size;
    thread->release(1);
    thread->returnFromFunction(ooData);
}

// MARK: Integer

void integerToString(Thread *thread) {
    EmojicodeInteger base = thread->getVariable(0).raw;
    EmojicodeInteger n = thread->getThisContext().value->raw, a = std::abs(n);
    bool negative = n < 0;

    EmojicodeInteger d = negative ? 2 : 1;
    while (n /= base) {
        d++;
    }

    Object *const &co = thread->retain(newArray(d * sizeof(EmojicodeChar)));

    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = d;
    string->charactersObject = co;

    EmojicodeChar *characters = string->characters() + d;
    do {
        *--characters =  "0123456789abcdefghijklmnopqrstuvxyz"[a % base % 35];
    } while (a /= base);

    if (negative) {
        characters[-1] = '-';
    }
    thread->release(1);
    thread->returnFromFunction(stringObject);
}

static void initPrngWithoutSeed(Thread *thread) {
    *thread->getThisObject()->val<std::mt19937_64>() = std::mt19937_64(std::random_device()());
    thread->returnFromFunction(thread->getThisContext());
}

static void prngIntegerUniform(Thread *thread) {
    auto dist = std::uniform_int_distribution<EmojicodeInteger>(thread->getVariable(0).raw, thread->getVariable(1).raw);
    thread->returnFromFunction(dist(*thread->getThisObject()->val<std::mt19937_64>()));
}

static void prngDoubleUniform(Thread *thread) {
    auto dist = std::uniform_real_distribution<double>();
    thread->returnFromFunction(dist(*thread->getThisObject()->val<std::mt19937_64>()));
}

static void integerAbsolute(Thread *thread) {
    thread->returnFromFunction(std::abs(thread->getThisContext().value->raw));
}

static void symbolToString(Thread *thread) {
    Object *co = thread->retain(newArray(sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = 1;
    string->charactersObject = co;
    thread->release(1);
    string->characters()[0] = thread->getThisContext().value->character;
    thread->returnFromFunction(stringObject);
}

static void symbolToInteger(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(thread->getThisContext().value->character));
}

static void doubleToString(Thread *thread) {
    EmojicodeInteger precision = thread->getVariable(0).raw;
    double d = thread->getThisContext().value->doubl;
    double absD = std::abs(d);

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

    Object *const &co = thread->retain(newArray(length * sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = length;
    string->charactersObject = co;
    thread->release(1);
    EmojicodeChar *characters = string->characters() + length;

    for (size_t i = precision; i > 0; i--) {
        *--characters = static_cast<unsigned char>(fmod(absD * pow(10, i), 10.0)) % 10 + '0';
    }

    if (precision != 0) {
        *--characters = '.';
    }

    for (size_t i = 0; i < iLength; i++) {
        *--characters =  static_cast<unsigned char>(fmod(absD / pow(10, i), 10.0)) % 10 + '0';
    }

    if (negative) {
        characters[-1] = '-';
    }
    thread->returnFromFunction(stringObject);
}

static void doubleSin(Thread *thread) {
    thread->returnFromFunction(sin(thread->getThisContext().value->doubl));
}

static void doubleCos(Thread *thread) {
    thread->returnFromFunction(cos(thread->getThisContext().value->doubl));
}

static void doubleTan(Thread *thread) {
    thread->returnFromFunction(tan(thread->getThisContext().value->doubl));
}

static void doubleASin(Thread *thread) {
    thread->returnFromFunction(asin(thread->getThisContext().value->doubl));
}

static void doubleACos(Thread *thread) {
    thread->returnFromFunction(acos(thread->getThisContext().value->doubl));
}

static void doubleATan(Thread *thread) {
    thread->returnFromFunction(atan(thread->getThisContext().value->doubl));
}

static void doublePow(Thread *thread) {
    thread->returnFromFunction(pow(thread->getThisContext().value->doubl, thread->getVariable(0).doubl));
}

static void doubleSqrt(Thread *thread) {
    thread->returnFromFunction(sqrt(thread->getThisContext().value->doubl));
}

static void doubleRound(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(round(thread->getThisContext().value->doubl)));
}

static void doubleCeil(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(ceil(thread->getThisContext().value->doubl)));
}

static void doubleFloor(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(floor(thread->getThisContext().value->doubl)));
}

static void doubleLog2(Thread *thread) {
    thread->returnFromFunction(log2(thread->getThisContext().value->doubl));
}

static void doubleLn(Thread *thread) {
    thread->returnFromFunction(log(thread->getThisContext().value->doubl));
}

static void doubleAbsolute(Thread *thread) {
    thread->returnFromFunction(fabs(thread->getThisContext().value->doubl));
}

// MARK: Callable

static void closureMark(Object *o) {
// TODO: Add correct implementation!
//    Closure *c = static_cast<Closure *>(o->value);
//    if (isRealObject(c->thisContext)) {
//        mark(&c->thisContext.object);
//    }
//    mark(&c->capturedVariables);
//
//    Value *t = static_cast<Value *>(c->capturedVariables->value);
//    CaptureInformation *infop = static_cast<CaptureInformation *>(c->capturesInformation->value);
//    for (unsigned int i = 0; i < c->captureCount; i++) {
//        Value *s = t + (infop++)->destination;
//        if (isRealObject(*s)) {
//            mark(&s->object);
//        }
//    }
}

FunctionFunctionPointer sLinkingTable[] = {
    nullptr,
    //📇
    dataEqual,
    dataSize,  // 🐔
    dataGetByte,  // 🐽
    dataToString,  // 🔡
    dataSlice,  // 🔪
    dataIndexOf,  // 🔍
    dataByAppendingData,  // 📝
    //💈
    initThread,
    nullptr,
    threadSleepMicroseconds,  // ⏲
    threadJoin,  // 🛂
    //🔐
    initMutex,
    mutexLock,  // 🔒
    mutexUnlock,  // 🔓
    mutexTryLock,  // 🔐
    //🚂
    integerToString,  // 🔡
    nullptr,  // 🎰
    integerAbsolute,  // 🏧
    doubleSin,  //📓
    doubleCos,  //📕
    doubleTan,  //📘
    doubleASin,  //📔
    doubleACos,  //📙
    doubleATan,  //📗
    doublePow,  //🏂
    doubleSqrt,  //⛷
    doubleCeil,  //🚴
    doubleFloor,  //🚵
    doubleRound,  //🏇
    doubleLog2,  //🚣
    doubleLn,  //🏄
    doubleToString,  //🔡
    doubleAbsolute,  //🏧
    //🔣
    symbolToString,  //🔡
    symbolToInteger,  //🚂
    //💻
    systemExit,  //🚪
    systemGetEnv,  //🌳
    systemCWD,
    systemTime,  //🕰
    systemArgs,  //🎞
    systemSystem,  //🕴
    nullptr,
    nullptr,
    nullptr,
    listAppendBridge,  // bear
    listGetBridge,  //🐽
    listRemoveBridge,  // koala
    listInsertBridge,  // monkey
    listCountBridge,  //🐔
    listPopBridge,  // panda
    listShuffleInPlaceBridge,  //🐹
    listFromListBridge,  //🐮
    listSort,  //🦁
    listRemoveAllBridge,  //🐗
    listSetBridge,  //🐷
    listEnsureCapacityBridge,  //🐴
    initListWithCapacity,  //🐧
    initListEmptyBridge,
    stringPrintStdoutBrigde,
    stringEqualBridge,  //🐔
    stringLengthBridge,  //📝
    stringByAppendingSymbolBridge,  //🐽
    stringSymbolAtBridge,  //🔪
    stringSubstringBridge,  //🔍
    stringIndexOf,  //🔧
    stringTrimBridge,  //🔫
    stringSplitByStringBridge,  //📐
    stringUTF8LengthBridge,  //💣
    stringSplitBySymbolBridge,  //🎼
    stringBeginsWithBridge,  //⛳️
    stringEndsWithBridge,  //🎶
    stringToCharacterList,  //📇
    stringToData,  //📰
    stringJSON,  //🚂
    stringToInteger,  //🚀
    stringToDouble,  //↔️
    stringCompareBridge,  //📫
    stringToUppercase,  //📪
    stringToLowercase,
    stringGetInput,  //😮
    stringFromSymbolListBridge,  //🎙
    stringFromStringList,  //🍨
    nullptr,
    nullptr,
    nullptr,
    initDictionaryBridge,
    bridgeDictionaryGet,  //🐽
    bridgeDictionaryRemove,  //🐨
    bridgeDictionarySet,  //🐷
    bridgeDictionaryKeys,  //🐙
    bridgeDictionaryClear,  //🐗
    bridgeDictionaryContains,  //🐣
    bridgeDictionarySize,  //🐔
    initPrngWithoutSeed,
    prngIntegerUniform,
    prngDoubleUniform,
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
        case 0x1f488:  //💈
            return sizeof(std::thread);
        case 0x1f510:  //🔐
            return sizeof(pthread_mutex_t);
        case 0x1f3b0:
            return sizeof(std::mt19937_64);
    }
    return 0;
}

Marker markerPointerForClass(EmojicodeChar cl) {
    switch (cl) {
        case 0x1F368:  // List
            return listMark;
        case 0x1F36F:  // Dictionary
            return dictionaryMark;
        case 0x1F521:
            return stringMark;
        case 0x1F347:
            return closureMark;
        case 0x1F4C7:
            return dataMark;
    }
    return nullptr;
}

}  // namespace Emojicode
