//
//  TokenStream.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TokenStream.hpp"
#include "CompilerError.hpp"

namespace EmojicodeCompiler {

Token TokenStream::consumeToken() {
    if (!hasMoreTokens()) {
        throw CompilerError(lexer_.position(), "Unexpected end of program.");
    }
    return advanceLexer();
}

Token TokenStream::consumeToken(TokenType type) {
    if (!hasMoreTokens()) {
        throw CompilerError(lexer_.position(), "Unexpected end of program.");
    }
    if (nextToken().type() != type) {
        throw CompilerError(nextToken().position(), "Expected ", Token::stringNameForType(type),
                            " but instead found ", nextToken().stringName(), "(", utf8(nextToken().value()), ").");
    }
    return advanceLexer();
}

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

Token TokenStream::advanceLexer() {
    skippedBlankLine_ = false;
    auto temp = std::move(nextToken_);
    while (true) {
        if (lexer_.continues()) {
            index_ = lexer_.index();
            nextToken_ = lexer_.lex();
            if (nextToken_.type() == TokenType::BlankLine) {
                skippedBlankLine_ = true;
                continue;
            }
            else if (nextToken_.type() == TokenType::SinglelineComment ||
                     nextToken_.type() == TokenType::MultilineComment) {
                nextToken_.position().file->addComment(std::move(nextToken_));
                continue;
            }
            else if (nextToken_.type() == TokenType::LineBreak) {
                continue;
            }
        }
        else {
            moreTokens_ = false;
        }
        break;
    }
    return temp;
}

}  // namespace EmojicodeCompiler
