//
//  Lexer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Lexer_hpp
#define Lexer_hpp

#include "EmojicodeCompiler.hpp"

//MARK: Tokens

enum TokenType {
    NO_TYPE,
    STRING,
    COMMENT,
    DOCUMENTATION_COMMENT,
    INTEGER,
    DOUBLE,
    BOOLEAN_TRUE,
    BOOLEAN_FALSE,
    IDENTIFIER,
    VARIABLE,
    SYMBOL,
    ARGUMENT_BRACKET_OPEN,
    ARGUMENT_BRACKET_CLOSE
};

struct SourcePosition {
    size_t line;
    size_t character;
    const char *file;
};

class Token {
public:
    Token() {}
    Token(Token *prevToken) {
        if(prevToken)
            prevToken->nextToken = this;
    }
    TokenType type = NO_TYPE;
    EmojicodeString value;
    SourcePosition position;
    Token *nextToken = nullptr;
    
    /** Returns a string describing the token */
    const char* stringName() const;
    
    /** Returns a string describing the token */
    static const char* stringNameForType(TokenType type);
    
    /** 
     * If this token is not of type @c type a compiler error is thrown.
     * @hint You should prefer @c consumeToken(TokenType type) over this method.
     * @see consumeToken(TokenType type)
     */
    void forceType(TokenType type) const;
    
    void validateInteger(bool hex) const;
    void validateDouble() const;
};

const Token* lex(FILE *f, const char* fileName);

/** 
 * Returns the next token or @c nullptr for the end of the stream.
 * @warning You may not call this function after it returned @c nullptr.
 */
const Token* consumeToken();
/**
 * Returns the next token if it matches the given type. If it does not match an error is issued.
 * It will never return @c nullptr.
 */
const Token* consumeToken(TokenType type);
/**
 * Returns the next token or @c nullptr for the end of the stream without consuming that token.
 */
const Token* nextToken();
extern const Token* currentToken;

#endif /* Lexer_hpp */
