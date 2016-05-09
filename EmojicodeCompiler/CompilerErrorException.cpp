//
//  CompilerErrorException.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 30/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstdarg>
#include "CompilerErrorException.hpp"

CompilerErrorException::CompilerErrorException(SourcePosition p, const char *err, ...) : position_(p) {
    va_list list;
    va_start(list, err);
    
    vsprintf(error_, err, list);
    
    va_end(list);
}
