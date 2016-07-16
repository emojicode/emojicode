//
//  TokenStream.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TokenStream.hpp"
#include "CompilerErrorException.hpp"

const Token& TokenStream::consumeToken(TokenType type) {
    if (!hasMoreTokens()) {
        throw CompilerErrorException(currentToken_->position(), "Unexpected end of program.");
    }
    currentToken_ = currentToken_->nextToken_;
    if (type != TokenType::NoType && currentToken_->type() != type) {
        throw CompilerErrorException(currentToken_->position(), "Expected token %s but instead found %s.",
                      Token::stringNameForType(type), currentToken_->stringName());
    }
    return *currentToken_;
}

bool TokenStream::hasMoreTokens() const {
    return currentToken_->nextToken_;
}

bool TokenStream::nextTokenIs(TokenType type) const {
    const Token *nextToken = currentToken_->nextToken_;
    return nextToken && nextToken->type() == type;
}

bool TokenStream::nextTokenIs(EmojicodeChar c) const {
    const Token *nextToken = currentToken_->nextToken_;
    return nextToken && nextToken->type() == TokenType::Identifier && nextToken->value[0] == c;
}

bool TokenStream::nextTokenIsEverythingBut(EmojicodeChar c) const {
    const Token *nextToken = currentToken_->nextToken_;
    return nextToken && !(nextToken->type() == TokenType::Identifier && nextToken->value[0] == c);
}