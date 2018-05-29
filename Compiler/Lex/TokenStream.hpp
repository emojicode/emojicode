//
//  TokenStream.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TokenStream_hpp
#define TokenStream_hpp

#include "CompilerError.hpp"
#include "Lexer.hpp"

namespace EmojicodeCompiler {

class Lexer;

/// TokenStream provides a convenient interface to Lexer. It allows a lookahead of one token.
/// TokenStream skips comments and line breaks and provides handling for blank lines.
class TokenStream {
public:
    explicit TokenStream(Lexer lexer) : lexer_(std::move(lexer)) { advanceLexer(); }
    TokenStream(const TokenStream&) = delete;
    TokenStream(TokenStream&&) = default;
    TokenStream& operator=(const TokenStream&) = delete;
    TokenStream& operator=(TokenStream&&) = delete;

    /// @returns The next token.
    /// @throws CompilerError if the end of the stream was reached.
    Token consumeToken() {
        if (!hasMoreTokens()) {
            throw CompilerError(lexer_.position(), "Unexpected end of program.");
        }
        return advanceLexer();
    }

    /// Returns the next token but only if it is of the specified type.
    /// @returns The next token.
    /// @throws CompilerError if the end of the stream was reached or the next token is not of the specified type.
    Token consumeToken(TokenType type) {
        if (!hasMoreTokens()) {
            throw CompilerError(lexer_.position(), "Unexpected end of program.");
        }
        if (nextToken().type() != type) {
            throw CompilerError(nextToken().position(), "Expected ", Token::stringNameForType(type),
                                " but instead found ", nextToken().stringName(), "(", utf8(nextToken().value()), ").");
        }
        return advanceLexer();
    }

    /// @returns True iff the end of the stream was reached and no more tokens are available.
    bool hasMoreTokens() const { return moreTokens_; }

    /// @returns True iff the end of the stream has not been reached and the next token is of the specified type.
    bool nextTokenIs(TokenType type) const { return hasMoreTokens() && nextToken().type() == type; }

    /// @returns True iff the end of the stream has not been reached and the next token is of the specified type and the
    ///          first value of the token is the specified one.
    bool nextTokenIs(char32_t c, TokenType type = TokenType::Identifier) const {
        return hasMoreTokens() && nextToken().type() == type && nextToken().value()[0] == c;
    }

    /// @returns The next token without consuming it.
    /// @pre hasMoreTokens() must return true.
    const Token &nextToken() const { return nextToken_; }

    /** 
     * Tests whether the end of the stream was not reached and the first element of the next token’s value does not
     * match the given character.
     */
    bool nextTokenIsEverythingBut(char32_t c, TokenType type = TokenType::Identifier) const {
        return hasMoreTokens() && !(nextToken().type() == type && nextToken().value()[0] == c);
    }

    bool nextTokenIsEverythingBut(TokenType type) const {
        return hasMoreTokens() && nextToken().type() != type;
    }

    /** Consumes the next token and returns true if it is an identifier and value’s first element matches
        the given character. */
    bool consumeTokenIf(char32_t c, TokenType type = TokenType::Identifier);

    /** Consumes the next token and returns true if it is an identifier and value’s first element matches
     the given character. */
    bool consumeTokenIf(TokenType type);

    /// Whether a blank line is between the last consumed token and nextToken().
    bool skipsBlankLine() const { return skippedBlankLine_; }

private:
    Token advanceLexer() {
        skippedBlankLine_ = false;
        auto temp = std::move(nextToken_);
        while (true) {
            if (lexer_.continues()) {
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

    bool moreTokens_ = true;
    bool skippedBlankLine_ = false;
    Lexer lexer_;
    Token nextToken_ = Token(SourcePosition(0, 0, nullptr));
};

}  // namespace EmojicodeCompiler

#endif /* TokenStream_hpp */
