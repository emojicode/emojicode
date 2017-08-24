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
        index_++;
        return true;
    }
    return false;
}

bool TokenStream::consumeTokenIf(TokenType type) {
    if (nextTokenIs(type)) {
        index_++;
        return true;
    }
    return false;
}

const Token& TokenStream::requireIdentifier(char32_t ch) {
    auto &token = consumeToken(TokenType::Identifier);
    if (!token.isIdentifier(ch)) {
        throw CompilerError(token.position(), "Expected ", utf8(std::u32string(1, ch)), " but found ",
                            utf8(token.value()), " instead.");
    }
    return token;
}

}  // namespace EmojicodeCompiler
