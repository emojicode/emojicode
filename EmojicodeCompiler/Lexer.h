//
//  Lexer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Lexer_h
#define Lexer_h

#include "EmojicodeCompiler.h"

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
    SYMBOL
};

struct SourcePosition {
    size_t line;
    size_t character;
    const char *file;
};

/**
 * A Token
 * @warning NEVER RELEASE A TOKEN!
 */
struct Token {
    Token() {}
    Token(Token *prevToken) {
        if(prevToken)
            prevToken->nextToken = this;
    }
    TokenType type = NO_TYPE;
    EmojicodeString value;
    SourcePosition position;
    struct Token *nextToken = NULL;
    
    /** Returns a string describing the token */
    const char* stringName() const;
    
    /** Returns a string describing the token */
    static const char* stringNameForType(TokenType type);
    
    /** If this token is not of type @c type a compiler error is thrown. */
    void forceType(TokenType type) const;
};

Token* lex(FILE *f, const char* fileName);

#endif /* Lexer_h */
