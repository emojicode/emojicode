//
//  Data.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 08/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Data.hpp"
#include "String.hpp"
#include "Thread.hpp"
#include "../utf8.h"
#include <algorithm>
#include <cstring>

namespace Emojicode {

void dataEqual(Thread *thread) {
    auto *d = thread->thisObject()->val<Data>();
    auto *b = thread->variable(0).object->val<Data>();

    if (d->length != b->length) {
        thread->returnFromFunction(false);
        return;
    }

    thread->returnFromFunction(memcmp(d->bytes, b->bytes, d->length) == 0);
}

void dataSize(Thread *thread) {
    thread->returnFromFunction(thread->thisObject()->val<Data>()->length);
}

void dataMark(Object *o) {
    auto *d = o->val<Data>();
    if (d->bytesObject != nullptr) {
        mark(&d->bytesObject);
        d->bytes = d->bytesObject->val<char>();
    }
}

void dataGetByte(Thread *thread) {
    auto *d = thread->thisObject()->val<Data>();

    EmojicodeInteger index = thread->variable(0).raw;
    if (index < 0) {
        index += d->length;
    }
    if (index < 0 || d->length <= index) {
        thread->returnNothingnessFromFunction();
        return;
    }

    thread->returnOEValueFromFunction(EmojicodeInteger(d->bytes[index]));
}

void dataToString(Thread *thread) {
    auto *data = thread->thisObject()->val<Data>();
    if (u8_isvalid(data->bytes, data->length) == 0) {
        thread->returnNothingnessFromFunction();
        return;
    }

    EmojicodeInteger len = u8_strlen_l(data->bytes, data->length);
    auto characters = thread->retain(newArray(len * sizeof(EmojicodeChar)));

    Object *sto = newObject(CL_STRING);
    auto *string = sto->val<String>();
    string->length = len;
    string->charactersObject = characters.unretainedPointer();
    thread->release(1);
    u8_toucs(string->characters(), len, data->bytes, data->length);
    thread->returnOEValueFromFunction(sto);
}

void dataSlice(Thread *thread) {
    Object *ooData = newObject(CL_DATA);
    auto *oData = ooData->val<Data>();
    auto *data = thread->thisObject()->val<Data>();

    EmojicodeInteger from = thread->variable(0).raw;
    if (from >= data->length) {
        thread->returnFromFunction(ooData);
        return;
    }

    EmojicodeInteger l = thread->variable(1).raw;
    if (thread->variable(0).raw + l > data->length) {
        l = data->length - thread->variable(0).raw;
    }

    oData->bytesObject = data->bytesObject;
    oData->bytes = data->bytes + from;
    oData->length = l;
    thread->returnFromFunction(ooData);
}

void dataIndexOf(Thread *thread) {
    auto *data = thread->thisObject()->val<Data>();
    auto *search = thread->variable(0).object->val<Data>();
    auto last = data->bytes + data->length;
    const void *location = std::search(data->bytes, last, search->bytes, search->bytes + search->length);
    if (location == last) {
        thread->returnNothingnessFromFunction();
    }
    else {
        thread->returnOEValueFromFunction(EmojicodeInteger((Byte *)location - (Byte *)data->bytes));
    }
}

void dataByAppendingData(Thread *thread) {
    auto *data = thread->thisObject()->val<Data>();
    auto *b = thread->variable(0).object->val<Data>();

    size_t size = data->length + b->length;
    auto newBytes = thread->retain(newArray(size));

    b = thread->variable(0).object->val<Data>();
    data = thread->thisObject()->val<Data>();

    std::memcpy(newBytes->val<char>(), data->bytes, data->length);
    std::memcpy(newBytes->val<char>() + data->length, b->bytes, b->length);

    Object *ooData = newObject(CL_DATA);
    auto *oData = ooData->val<Data>();
    oData->bytesObject = newBytes.unretainedPointer();
    oData->bytes = oData->bytesObject->val<char>();
    oData->length = size;
    thread->release(1);
    thread->returnFromFunction(ooData);
}

}  // namespace Emojicode
