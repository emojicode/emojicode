//
//  VariableNotFoundErrorException.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef VariableNotFoundErrorException_hpp
#define VariableNotFoundErrorException_hpp

#include "CompilerErrorException.hpp"

class VariableNotFoundErrorException: public CompilerErrorException {
public:
    VariableNotFoundErrorException(SourcePosition p, const EmojicodeString &name)
        : CompilerErrorException(p, "Variable \"%s\" not defined.", name.utf8CString()) {};
};

#endif /* VariableNotFoundErrorException_hpp */
