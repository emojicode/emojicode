//
//  TokenStream.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TokenStream_hpp
#define TokenStream_hpp

#include "Token.hpp"

class TokenStream {
    friend TokenStream lex(const char *);
public:
    TokenStream() : currentToken_(nullptr) {};
    /**
     * Returns the next token if it matches the given type. If it does not match or the end of the stream was reached
     * an error is issued.
     */
    const Token& consumeToken(TokenType type = TokenType::NoType);
    /** Whether there are tokens left or the end of the stream was reached. */
    bool hasMoreTokens() const;
    /** Tests whether the next token is of the given type. */
    bool nextTokenIs(TokenType type) const;
    /** Tests whether the next token is an identifier and the value’s first element matches the given character. */
    bool nextTokenIs(EmojicodeChar c) const;
    /** 
     * Tests whether the end of the stream was not reached and the first element of the next token’s value does not
     * match the given character.
     */
    bool nextTokenIsEverythingBut(EmojicodeChar c) const;
private:
    TokenStream(Token *beginToken) : currentToken_(beginToken) {};
    const Token* currentToken_;
};

#endif /* TokenStream_hpp */
