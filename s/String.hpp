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
#include "../runtime/Runtime.h"

namespace s {

class String : public runtime::Object<String>  {
public:
    using Character = runtime::Symbol;

    Character *characters;
    runtime::Integer count;

    const char* cString();
    int compare(String *other);
};

}

SET_META_FOR(s::String, s, 1f521)

#endif /* String_hpp */
