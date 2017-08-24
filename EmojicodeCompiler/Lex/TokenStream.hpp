//
//  TokenStream.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TokenStream_hpp
#define TokenStream_hpp

#include "../CompilerError.hpp"
#include "Token.hpp"
#include <vector>

namespace EmojicodeCompiler {

class RecompilationPoint;

class TokenStream {
    friend RecompilationPoint;
    friend TokenStream lex(const std::string &);
public:
    explicit TokenStream(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    /**
     * Returns the next token if it matches the given type. If it does not match or the end of the stream was reached
     * an error is issued.
     */
    const Token& consumeToken() {
        if (!hasMoreTokens()) {
            throw CompilerError(nextToken().position(), "Unexpected end of program.");
        }
        return tokens_[index_++];
    }

    template<class... Args>
    const Token& consumeToken(Args... args) {
        if (!hasMoreTokens()) {
            throw CompilerError(nextToken().position(), "Unexpected end of program.");
        }
        if (!checkType(nextToken().type(), args...)) {
            throw CompilerError(nextToken().position(), "Expected token but instead found %s (%s).",
                                /*Token::stringNameForType(type),*/ nextToken().stringName(),
                                utf8(nextToken().value()).c_str());
        }
        return tokens_[index_++];
    }

    /** Whether there are tokens left or the end of the stream was reached. */
    bool hasMoreTokens() const { return index_ < tokens_.size(); }
    /** Tests whether the next token is of the given type. */
    bool nextTokenIs(TokenType type) const { return hasMoreTokens() && nextToken().type() == type; }
    /** Tests whether the next token is an identifier and the value’s first element matches the given character. */
    bool nextTokenIs(char32_t c, TokenType type = TokenType::Identifier) const {
        return hasMoreTokens() && nextToken().type() == type && nextToken().value()[0] == c;
    }
    const Token& nextToken() const { return tokens_[index_]; }
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

    /// Consumes the next toekn and throws an CompilerError if this token isn’t an indentifier whose
    /// value is @c ch.
    const Token& requireIdentifier(char32_t ch);
private:
    std::vector<Token> tokens_;
    size_t index_ = 0;

    template<class... Args>
    bool checkType(TokenType type, TokenType head, Args... args) {
        return type == head || checkType(type, args...);
    }

    bool checkType(TokenType type, TokenType head) {
        return type == head;
    }
};

}  // namespace EmojicodeCompiler

#endif /* TokenStream_hpp */
