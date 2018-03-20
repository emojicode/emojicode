//
// Created by Theo Weidmann on 19.03.18.
//

#include "../runtime/Runtime.h"
#include "Data.hpp"
#include "String.hpp"
#include "utf8.h"
#include <algorithm>

namespace s {

extern "C" runtime::SimpleOptional<runtime::Integer> sDataFindFromIndex(Data *data, Data *search,
                                                                        runtime::Integer offset) {
    if (offset >= data->count) {
        return runtime::NoValue;
    }
    auto end = data->data + data->count;
    auto pos = std::search(data->data + offset, end, search->data, search->data + search->count);
    if (pos != end) {
        return pos - data->data;
    }
    return runtime::NoValue;
}

extern "C" runtime::SimpleOptional<String *> sDataAsString(Data *data) {
    auto chars = reinterpret_cast<char *>(data->data);
    if (!u8_isvalid(chars, data->count)) {
        return runtime::NoValue;
    }

    auto *string = String::allocateAndInitType();
    string->count = u8_strlen_l(chars, data->count);

    if (data->count == 0) {
        return string;
    }

    string->characters = runtime::allocate<String::Character>(string->count);
    u8_toucs(string->characters, string->count, chars, data->count);

    return string;
}

}  // namespace s