//
//  TokenStream.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TokenStream.hpp"

namespace EmojicodeCompiler {

bool TokenStream::consumeTokenIf(char32_t c, TokenType type) {
    if (nextTokenIs(c, type)) {
        advanceLexer();
        return true;
    }
    return false;
}

bool TokenStream::consumeTokenIf(TokenType type) {
    if (nextTokenIs(type)) {
        advanceLexer();
        return true;
    }
    return false;
}

}  // namespace EmojicodeCompiler
