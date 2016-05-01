//
//  CompilerErrorException.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CompilerErrorException_hpp
#define CompilerErrorException_hpp

#include <exception>
#include "Token.hpp"

class CompilerErrorException: public std::exception {
public:
    CompilerErrorException(SourcePosition p, const char *err, ...);
    const SourcePosition& position() const { return position_; }
    const char* error() const { return error_; }
private:
    SourcePosition position_;
    char error_[450];
};

#endif /* CompilerErrorException_hpp */
