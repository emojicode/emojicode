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
    using Character = uint32_t;

    void *meta;
    const Character *characters;
    const int64_t count;

    const char* cString();
    int compare(String *other);
};

}

#endif /* String_hpp */
