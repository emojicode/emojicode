//
//  Variable.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Variable.hpp"
#include "CompilerError.hpp"

namespace EmojicodeCompiler {

void Variable::mutate(const SourcePosition &p) {
    if (constant()) {
        throw CompilerError(p, "Cannot modify constant variable \"", utf8(name()), "\".");
    }
    mutated_ = true;
}

}  // namespace EmojicodeCompiler
