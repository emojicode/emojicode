//
//  Variable.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Variable.hpp"
#include "CompilerError.hpp"

void Variable::uninitalizedError(SourcePosition p) const {
    if (initialized_ <= 0) {
        throw CompilerError(position(), "Variable \"%s\" is possibly not initialized.", name().utf8().c_str());
    }
}

void Variable::mutate(SourcePosition p) {
    if (frozen()) {
        throw CompilerError(position(), "Cannot modify frozen variable \"%s\".", name().utf8().c_str());
    }
    mutated_ = true;
}
