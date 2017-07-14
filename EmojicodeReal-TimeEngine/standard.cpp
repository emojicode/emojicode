//
//  ElementaryTypes.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "standard.hpp"
#include "../utf8.h"
#include "Dictionary.hpp"
#include "Engine.hpp"
#include "List.hpp"
#include "String.hpp"
#include "Data.hpp"
#include "Thread.hpp"
#include "ThreadsManager.hpp"
#include "Memory.hpp"
#include "Class.hpp"
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
    EmojicodeInteger state = thread->variable(0).raw;
    exit((int)state);
}

static void systemGetEnv(Thread *thread) {
    char *env = getenv(stringToCString(thread->variable(0).object));

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
    auto listObject = thread->retain(newObject(CL_LIST));

    auto *newList = listObject->val<List>();
    newList->capacity = cliArgumentCount;
    Object *items = newArray(sizeof(Value) * cliArgumentCount);

    listObject->val<List>()->items = items;

    for (int i = 0; i < cliArgumentCount; i++) {
        listAppendDestination(listObject, thread)->copySingleValue(T_OBJECT, stringFromChar(cliArguments[i]));
    }

    thread->release(1);
    thread->returnFromFunction(listObject.unretainedPointer());
}

static void systemSystem(Thread *thread) {
    FILE *f = popen(stringToCString(thread->variable(0).object), "r");

    if (f == nullptr) {
        thread->returnNothingnessFromFunction();
        return;
    }

    size_t bufferUsedSize = 0;
    int bufferSize = 50;
    auto buffer = thread->retain(newArray(bufferSize));

    while (fgets(buffer->val<char>() + bufferUsedSize, bufferSize - (int)bufferUsedSize, f) != nullptr) {
        bufferUsedSize = strlen(buffer->val<char>());

        if (bufferSize - bufferUsedSize < 2) {
            bufferSize *= 2;
            buffer = resizeArray(buffer.unretainedPointer(), bufferSize, thread);
        }
    }

    bufferUsedSize = strlen(buffer->val<char>());

    EmojicodeInteger len = u8_strlen_l(buffer->val<char>(), bufferUsedSize);

    auto so = thread->retain(newObject(CL_STRING));
    auto *string = so->val<String>();
    string->length = len;

    Object *chars = newArray(len * sizeof(EmojicodeChar));
    string = so->val<String>();
    string->charactersObject = chars;

    u8_toucs(string->characters(), len, buffer->val<char>(), bufferUsedSize);
    thread->release(2);
    thread->returnFromFunction(so.unretainedPointer());
}

//MARK: Threads

static void threadJoin(Thread *thread) {
    auto cthread = *thread->thisObject()->val<std::thread*>();
    allowGC();
    cthread->join();
    disallowGCAndPauseIfNeeded();
    thread->returnFromFunction();
}

static void threadSleepMicroseconds(Thread *thread) {
    std::this_thread::sleep_for(std::chrono::microseconds(thread->variable(0).raw));
    thread->returnFromFunction();
}

void threadStart(Thread *thread, RetainedObjectPointer callable) {
    thread->release(1);
    executeCallableExtern(callable.unretainedPointer(), nullptr, 0, thread, nullptr);
    ThreadsManager::deallocateThread(thread);
}

static void initThread(Thread *thread) {
    auto newThread = ThreadsManager::allocateThread();
    auto callable = thread->variable(0).object;
    *thread->thisObject()->val<std::thread*>() = new std::thread(threadStart, newThread, newThread->retain(callable));
    registerForDeinitialization(thread->thisObject());
    thread->returnFromFunction(thread->thisContext());
}

static void initMutex(Thread *thread) {
    pthread_mutex_init(thread->thisObject()->val<pthread_mutex_t>(), nullptr);
    thread->returnFromFunction(thread->thisContext());
}

static void mutexLock(Thread *thread) {
    while (pthread_mutex_trylock(thread->thisObject()->val<pthread_mutex_t>()) != 0) {
        // TODO: Obviously stupid, but this is the only safe way. If pthread_mutex_lock was used,
        // the thread would be block, and the GC could cause a deadlock. allowGC, however, would
        // allow moving this mutex – obviously not a good idea either when using pthread_mutex_lock.
        pauseForGC();
        usleep(10);
    }
    thread->returnFromFunction();
}

static void mutexUnlock(Thread *thread) {
    pthread_mutex_unlock(thread->thisObject()->val<pthread_mutex_t>());
    thread->returnFromFunction();
}

static void mutexTryLock(Thread *thread) {
    thread->returnFromFunction(pthread_mutex_trylock(thread->thisObject()->val<pthread_mutex_t>()) == 0);
}

// MARK: Integer

void integerToString(Thread *thread) {
    EmojicodeInteger base = thread->variable(0).raw;
    EmojicodeInteger n = thread->thisContext().value->raw, a = std::abs(n);
    bool negative = n < 0;

    EmojicodeInteger d = negative ? 2 : 1;
    while (n /= base) {
        d++;
    }

    auto co = thread->retain(newArray(d * sizeof(EmojicodeChar)));

    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = d;
    string->charactersObject = co.unretainedPointer();

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
    *thread->thisObject()->val<std::mt19937_64>() = std::mt19937_64(std::random_device()());
    thread->returnFromFunction(thread->thisContext());
}

static void prngIntegerUniform(Thread *thread) {
    auto dist = std::uniform_int_distribution<EmojicodeInteger>(thread->variable(0).raw, thread->variable(1).raw);
    thread->returnFromFunction(dist(*thread->thisObject()->val<std::mt19937_64>()));
}

static void prngDoubleUniform(Thread *thread) {
    auto dist = std::uniform_real_distribution<double>();
    thread->returnFromFunction(dist(*thread->thisObject()->val<std::mt19937_64>()));
}

static void integerAbsolute(Thread *thread) {
    thread->returnFromFunction(std::abs(thread->thisContext().value->raw));
}

static void symbolToString(Thread *thread) {
    auto co = thread->retain(newArray(sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = 1;
    string->charactersObject = co.unretainedPointer();
    thread->release(1);
    string->characters()[0] = thread->thisContext().value->character;
    thread->returnFromFunction(stringObject);
}

static void symbolToInteger(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(thread->thisContext().value->character));
}

static void doubleToString(Thread *thread) {
    EmojicodeInteger precision = thread->variable(0).raw;
    double d = thread->thisContext().value->doubl;
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

    auto co = thread->retain(newArray(length * sizeof(EmojicodeChar)));
    Object *stringObject = newObject(CL_STRING);
    auto *string = stringObject->val<String>();
    string->length = length;
    string->charactersObject = co.unretainedPointer();
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
    thread->returnFromFunction(sin(thread->thisContext().value->doubl));
}

static void doubleCos(Thread *thread) {
    thread->returnFromFunction(cos(thread->thisContext().value->doubl));
}

static void doubleTan(Thread *thread) {
    thread->returnFromFunction(tan(thread->thisContext().value->doubl));
}

static void doubleASin(Thread *thread) {
    thread->returnFromFunction(asin(thread->thisContext().value->doubl));
}

static void doubleACos(Thread *thread) {
    thread->returnFromFunction(acos(thread->thisContext().value->doubl));
}

static void doubleATan(Thread *thread) {
    thread->returnFromFunction(atan(thread->thisContext().value->doubl));
}

static void doublePow(Thread *thread) {
    thread->returnFromFunction(pow(thread->thisContext().value->doubl, thread->variable(0).doubl));
}

static void doubleSqrt(Thread *thread) {
    thread->returnFromFunction(sqrt(thread->thisContext().value->doubl));
}

static void doubleRound(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(round(thread->thisContext().value->doubl)));
}

static void doubleCeil(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(ceil(thread->thisContext().value->doubl)));
}

static void doubleFloor(Thread *thread) {
    thread->returnFromFunction(static_cast<EmojicodeInteger>(floor(thread->thisContext().value->doubl)));
}

static void doubleLog2(Thread *thread) {
    thread->returnFromFunction(log2(thread->thisContext().value->doubl));
}

static void doubleLn(Thread *thread) {
    thread->returnFromFunction(log(thread->thisContext().value->doubl));
}

static void doubleAbsolute(Thread *thread) {
    thread->returnFromFunction(fabs(thread->thisContext().value->doubl));
}

// MARK: Callable

static void closureMark(Object *o) {
    auto c = o->val<Closure>();
    if (c->thisContext.object != nullptr) {
        mark(&c->thisContext.object);
    }
    mark(&c->capturedVariables);
    mark(&c->capturesInformation);

    auto value = c->capturedVariables->val<Value>();
    auto records = c->capturesInformation->val<ObjectVariableRecord>();
    for (size_t i = 0; i < c->recordsCount; i++) {
        markByObjectVariableRecord(records[i], value, i);
    }
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

void sPrepareClass(Class *klass, EmojicodeChar name) {
    switch (name) {
        case 0x1F521:
            klass->valueSize = sizeof(String);
            klass->mark = stringMark;
            break;
        case 0x1F368:
            klass->valueSize = sizeof(List);
            klass->mark = listMark;
            break;
        case 0x1F36F:
            klass->valueSize = sizeof(EmojicodeDictionary);
            klass->mark = dictionaryMark;
            break;
        case 0x1F4C7:
            klass->valueSize = sizeof(Data);
            klass->mark = dataMark;
            break;
        case 0x1F347:
            klass->valueSize = sizeof(Closure);
            klass->mark = closureMark;
            break;
        case 0x1f488:  //💈
            klass->valueSize = sizeof(std::thread*);
            klass->deinit = [](Object *o) {
                auto thread = *o->val<std::thread*>();
                thread->detach();
                delete thread;
            };
            break;
        case 0x1f510:  //🔐
            klass->valueSize = sizeof(pthread_mutex_t);
            break;
        case 0x1f3b0:
            klass->valueSize = sizeof(std::mt19937_64);
            break;
    }
}

}  // namespace Emojicode
