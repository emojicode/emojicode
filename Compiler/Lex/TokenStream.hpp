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
#include "Token.hpp"
#include <vector>

namespace EmojicodeCompiler {

/// This class manages the Tokens for the parser. Allows one token of lookahead.
class TokenStream {
public:
    explicit TokenStream(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    /// @returns The next token.
    /// @throws CompilerError if the end of the stream was reached.
    const Token &consumeToken() {
        if (!hasMoreTokens()) {
            throw CompilerError(nextToken().position(), "Unexpected end of program.");
        }
        return tokens_[index_++];
    }

    /// Returns the next token but only if it is of the specified type.
    /// @returns The next token.
    /// @throws CompilerError if the end of the stream was reached or the next token is not of the specified type.
    const Token &consumeToken(TokenType type) {
        if (!hasMoreTokens()) {
            throw CompilerError(tokens_[index_ - 1].position(), "Unexpected end of program.");
        }
        if (nextToken().type() != type) {
            throw CompilerError(nextToken().position(), "Expected ", Token::stringNameForType(type),
                                " but instead found ", nextToken().stringName(), "(", utf8(nextToken().value()), ").");
        }
        return tokens_[index_++];
    }

    /// @returns True iff the end of the stream was reached and no more tokens are available.
    bool hasMoreTokens() const { return index_ < tokens_.size(); }

    /// @returns True iff the end of the stream has not been reached and the next token is of the specified type.
    bool nextTokenIs(TokenType type) const { return hasMoreTokens() && nextToken().type() == type; }

    /// @returns True iff the end of the stream has not been reached and the next token is of the specified type and the
    ///          first value of the token is the specified one.
    bool nextTokenIs(char32_t c, TokenType type = TokenType::Identifier) const {
        return hasMoreTokens() && nextToken().type() == type && nextToken().value()[0] == c;
    }

    /// @returns The next token.
    /// @pre hasMoreTokens() must return true.
    const Token &nextToken() const { return tokens_[index_]; }

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

private:
    std::vector<Token> tokens_;
    size_t index_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* TokenStream_hpp */
