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

void Variable::uninitalizedError(const SourcePosition &p) const {
    if (initialized_ <= 0) {
        throw CompilerError(p, "Variable \"", utf8(name()),"\" is possibly not initialized.");
    }
}

void Variable::mutate(const SourcePosition &p) {
    if (constant()) {
        throw CompilerError(p, "Cannot modify constant variable \"", utf8(name()), "\".");
    }
    mutated_ = true;
}

}  // namespace EmojicodeCompiler
