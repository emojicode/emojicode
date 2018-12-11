//
//  TokenStream.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TokenStream_hpp
#define TokenStream_hpp

#include "Lexer.hpp"

namespace EmojicodeCompiler {

/// TokenStream provides a convenient interface to Lexer. It allows a lookahead of one token.
/// TokenStream skips comments and line breaks and provides handling for blank lines.
class TokenStream {
public:
    explicit TokenStream(Lexer lexer) : lexer_(std::move(lexer)) { advanceLexer(); }
    TokenStream(const TokenStream&) = delete;
    TokenStream(TokenStream&&) = default;
    TokenStream& operator=(const TokenStream&) = delete;
    TokenStream& operator=(TokenStream&&) = delete;

    /// Consumes the next token.
    /// @throws CompilerError if the end of the stream was reached.
    Token consumeToken();

    /// Consumes the next token while making sure that it is of the specified type.
    /// @throws CompilerError if the end of the stream was reached or the next token is not of the specified type.
    Token consumeToken(TokenType type);

    /// @returns True iff the more tokens are available.
    bool hasMoreTokens() const { return moreTokens_; }

    /// @returns The next token without consuming it.
    /// @pre hasMoreTokens() must return true.
    const Token& nextToken() const { return nextToken_; }

    /// @returns True iff the end of the stream has not been reached and the next token is of the specified type.
    bool nextTokenIs(TokenType type) const { return hasMoreTokens() && nextToken().type() == type; }

    /// @returns True iff the end of the stream has not been reached and the next token is an identifer and the
    ///          first value of the token is the specified one.
    bool nextTokenIs(char32_t c) const {
        return hasMoreTokens() && nextToken().type() == TokenType::Identifier && nextToken().value()[0] == c;
    }

    /// Returns true iff the stream has more tokens and the next token does not match the provided value as determined
    /// by nextTokenIs(char32_t).
    bool nextTokenIsEverythingBut(char32_t c) const {
        return hasMoreTokens() && !(nextToken().type() == TokenType::Identifier && nextToken().value()[0] == c);
    }

    /// Returns true iff the stream has more tokens and the next token is not of the specified type.
    bool nextTokenIsEverythingBut(TokenType type) const {
        return hasMoreTokens() && nextToken().type() != type;
    }

    /// Consumes the next token and returns true iff by nextTokenIs(char32_t) returns true for the provided value.
    bool consumeTokenIf(char32_t c);

    /// Consumes the next token and returns true iff by nextTokenIs(TokenType) returns true for the provided value.
    bool consumeTokenIf(TokenType type);

    /// Whether a blank line is between the last consumed token and nextToken().
    bool skipsBlankLine() const { return skippedBlankLine_; }

    size_t index() const { return index_; }

private:
    Token advanceLexer();

    bool moreTokens_ = true;
    bool skippedBlankLine_ = false;
    Lexer lexer_;
    Token nextToken_ = Token(SourcePosition());
    size_t index_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* TokenStream_hpp */
