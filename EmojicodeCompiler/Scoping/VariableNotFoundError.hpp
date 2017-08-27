//
//  VariableNotFoundError.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef VariableNotFoundError_hpp
#define VariableNotFoundError_hpp

#include "../CompilerError.hpp"
#include <utility>

namespace EmojicodeCompiler {

class VariableNotFoundError: public CompilerError {
public:
    VariableNotFoundError(SourcePosition p, const std::u32string &name)
        : CompilerError(std::move(p), "Variable \"%s\" not defined.", utf8(name).c_str()) {};
};

}  // namespace EmojicodeCompiler

#endif /* VariableNotFoundError_hpp */
