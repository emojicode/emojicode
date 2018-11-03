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
    String(const char *string);
    /// Creates a string without a value.
    /// @warning A string object created with this constructor is not ready for use!
    String() = default;

    /// This method can be used to make a newly constructed string represent the value of the provided string.
    /// @warning Do not use this method to modify an existing string, i.e. one that has a value already.
    void store(const char *cstring);

    runtime::MemoryPointer<char> characters;
    runtime::Integer count;

    std::string stdString();
    int compare(String *other);
};

}  // namespace s

SET_INFO_FOR(s::String, s, 1f521)

#endif /* String_hpp */
