//
// Created by Theo Weidmann on 18.03.18.
//

#include "../runtime/Runtime.h"

extern "C" runtime::Integer sSymbolToInt(runtime::Symbol *symbol) {
    return static_cast<runtime::Integer>(*symbol);
}