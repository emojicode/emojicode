//
//  TokenStream.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TokenStream.hpp"
#include "CompilerError.hpp"

const Token& TokenStream::consumeToken(TokenType type) {
    if (!hasMoreTokens()) {
        throw CompilerError(nextToken().position(), "Unexpected end of program.");
    }

    if (type != TokenType::NoType && nextToken().type() != type) {
        throw CompilerError(nextToken().position(), "Expected token %s but instead found %s.",
                                     Token::stringNameForType(type), nextToken().stringName());
    }
    return (*tokens_)[index_++];
}

bool TokenStream::nextTokenIsEverythingBut(EmojicodeChar c) const {
    return hasMoreTokens() && !(nextToken().type() == TokenType::Identifier && nextToken().value()[0] == c);
}

bool TokenStream::consumeTokenIf(EmojicodeChar c) {
    if (nextTokenIs(c)) {
        index_++;
        return true;
    }
    return false;
}

const Token& TokenStream::requireIdentifier(EmojicodeChar ch) {
    auto &token = consumeToken(TokenType::Identifier);
    if (!token.isIdentifier(ch)) {
        throw CompilerError(token.position(), "Expected %s but found %s instead.", EmojicodeString(ch).utf8().c_str(),
                            token.value().utf8().c_str());
    }
    return token;
}
