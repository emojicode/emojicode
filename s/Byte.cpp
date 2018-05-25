//
// Created by Theo Weidmann on 25.05.18.
//

#include "../runtime/Runtime.h"

extern "C" runtime::Integer sByteToInt(runtime::Byte *byte) {
    return static_cast<runtime::Integer>(*byte);
}
