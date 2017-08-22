//
//  CompilerError.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CompilerError_hpp
#define CompilerError_hpp

#include <exception>
#include "Lex/SourcePosition.hpp"

namespace EmojicodeCompiler {

class CompilerError: public std::exception {
public:
    CompilerError(SourcePosition p, const char *err, ...);
    const SourcePosition& position() const { return position_; }
    const char* error() const { return error_; }
private:
    SourcePosition position_;
    char error_[450];
};

}  // namespace EmojicodeCompiler

#endif /* CompilerError_hpp */
