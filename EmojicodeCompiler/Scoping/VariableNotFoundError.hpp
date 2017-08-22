//
//  VariableNotFoundError.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef VariableNotFoundError_hpp
#define VariableNotFoundError_hpp

#include "../EmojicodeCompiler.hpp"
#include "../CompilerError.hpp"

namespace EmojicodeCompiler {

class VariableNotFoundError: public CompilerError {
public:
    VariableNotFoundError(SourcePosition p, const EmojicodeString &name)
        : CompilerError(p, "Variable \"%s\" not defined.", name.utf8().c_str()) {};
};

}  // namespace EmojicodeCompiler

#endif /* VariableNotFoundError_hpp */
