//
// Created by Theo Weidmann on 19.03.18.
//

#include "../runtime/Runtime.h"
#include "Data.h"
#include "String.h"
#include "utf8.h"
#include <algorithm>

namespace s {

extern "C" runtime::SimpleOptional<runtime::Integer> sDataFindFromIndex(Data *data, Data *search,
                                                                        runtime::Integer offset) {
    if (offset >= data->count) {
        return runtime::NoValue;
    }
    auto end = data->data.get() + data->count;
    auto pos = std::search(data->data.get() + offset, end, search->data.get(), search->data.get() + search->count);
    if (pos != end) {
        return pos - data->data.get();
    }
    return runtime::NoValue;
}

extern "C" runtime::SimpleOptional<String *> sDataAsString(Data *data) {
    auto chars = reinterpret_cast<char *>(data->data.get());
    if (!u8_isvalid(chars, data->count)) {
        return runtime::NoValue;
    }

    auto *string = String::init();
    string->count = u8_strlen_l(chars, data->count);

    string->characters = runtime::allocate<String::Character>(string->count);
    u8_toucs(string->characters.get(), string->count, chars, data->count);
    return string;
}

}  // namespace s
