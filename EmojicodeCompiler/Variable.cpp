//
//  Variable.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Variable.hpp"
#include "CompilerErrorException.hpp"

void Variable::uninitalizedError(const Token &variableToken) const {
    if (initialized <= 0) {
        throw CompilerErrorException(variableToken,
                                     "Variable \"%s\" is possibly not initialized.", variableToken.value.utf8CString());
    }
}

void Variable::mutate(const Token &variableToken) {
    if (frozen()) {
        throw CompilerErrorException(variableToken, "Cannot modify frozen variable \"%s\".",
                                     variableToken.value.utf8CString());
    }
    mutated_ = true;
}