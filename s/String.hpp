//
//  String.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef String_hpp
#define String_hpp

#include <cstdint>

namespace s {

struct String {
    void *meta;
    uint32_t *characters;
    int64_t count;

    const char* cString();
};

}

#endif /* String_hpp */
