//
// Created by Theo Weidmann on 19.03.18.
//

#include "../runtime/Runtime.h"
#include "Data.h"
#include "String.h"
#include "utf8proc.h"
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

    // TODO: validate

    auto *string = String::init();
    string->count = data->count;
    string->characters = data->data;
    data->data.retain();
    return string;
}

}  // namespace s
