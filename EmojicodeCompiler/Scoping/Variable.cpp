//
//  Variable.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Variable.hpp"
#include "../CompilerError.hpp"

namespace EmojicodeCompiler {

void Variable::uninitalizedError(const SourcePosition &p) const {
    if (initialized_ <= 0) {
        throw CompilerError(p, "Variable \"%s\" is possibly not initialized.", name().utf8().c_str());
    }
}

void Variable::mutate(const SourcePosition &p) {
    if (frozen()) {
        throw CompilerError(p, "Cannot modify frozen variable \"%s\".", name().utf8().c_str());
    }
    mutated_ = true;
}

}  // namespace EmojicodeCompiler
