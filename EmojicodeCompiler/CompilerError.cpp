//
//  CompilerError.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CompilerError.hpp"
#include <cstdarg>

namespace EmojicodeCompiler {

CompilerError::CompilerError(SourcePosition p, const char *err, ...) : position_(std::move(p)) {
    va_list list;
    va_start(list, err);

    vsprintf(error_, err, list);

    va_end(list);
}

}  // namespace EmojicodeCompiler
