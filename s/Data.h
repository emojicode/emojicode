//
// Created by Theo Weidmann on 19.03.18.
//

#ifndef EMOJICODE_DATA_HPP
#define EMOJICODE_DATA_HPP

namespace s {

#include "../runtime/Runtime.h"

class Data : public runtime::Object<Data>  {
public:
    runtime::MemoryPointer<runtime::Byte> data;
    runtime::Integer count;
};

}  // namespace s

SET_INFO_FOR(s::Data, s, 1f4c7)

#endif //EMOJICODE_DATA_HPP
