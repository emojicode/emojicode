//
//  String.
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 11/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef String_hpp
#define String_hpp

#include <cstdint>
#include <string>
#include "../runtime/Runtime.h"

namespace s {

class String : public runtime::Object<String>  {
public:
    using Character = runtime::Symbol;

    String(const char *string);
    String() = default;

    runtime::MemoryPointer<Character> characters;
    runtime::Integer count;

    std::string stdString();
    int compare(String *other);
};

}  // namespace s

SET_INFO_FOR(s::String, s, 1f521)

#endif /* String_hpp */
