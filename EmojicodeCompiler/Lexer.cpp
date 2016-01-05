//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include "utf8.h"

#define isNewline() (c == 0x0A || c == 0x2028 || c == 0x2029)

bool detectWhitespace(EmojicodeChar c, size_t *col, size_t *line){
    if(isNewline()){
        *col = 0;
        (*line)++;
        return true;
    }
    return isWhitespace(c);
}

Token* lex(FILE *f, const char *filename) {
    EmojicodeChar c;
    size_t line = 1, col = 0, i = 0;
    SourcePosition sourcePosition;
    
    Token *token, *firstToken;
    token = firstToken = new Token();
    
    bool nextToken = false;
    bool oneLineComment = false;
    bool isHex = false;
    bool escapeSequence = false;
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    auto stringBuffer = new char[length + 1];
    if (stringBuffer){
        fread(stringBuffer, 1, length, f);
        stringBuffer[length] = 0;
    }
    else {
        compilerError(NULL, "Cannot allocate buffer for file %s. It is possibly to large.", filename);
    }

#define isIdentifier() ((0x1F300 <= c && c <= 0x1F64F) || (0x1F680 <= c && c <= 0x1F6C5) || (0x2600 <= c && c <= 0x27BF) || (0x1F191 <= c && c <= 0x1F19A) || c == 0x231A || (0x1F910 <= c && c <= 0x1F9C0) || (0x2B00 <= c && c <= 0x2BFF) || (0x25A0 <= c && c <= 0x25FF) || (0x2300 <= c && c <= 0x23FF))
    
    while(i < length){
        c = u8_nextchar(stringBuffer, &i);
        col++;
        sourcePosition = (SourcePosition){line, col, filename};
        
        /* We already know this token’s type. */
        if (token->type == COMMENT){
            if(!oneLineComment){
                detectWhitespace(c, &col, &line);
                if (c == E_OLDER_WOMAN){
                    /* End of the comment, reset the token for future use */
                    token->type = NO_TYPE;
                }
            }
            else {
                detectWhitespace(c, &col, &line);
                if(isNewline()){
                    token->type = NO_TYPE;
                }
            }
            continue;
        }
        else if (token->type == DOCUMENTATION_COMMENT && !nextToken){
            detectWhitespace(c, &col, &line);
            if(c == E_TACO){
                nextToken = true;
            }
            else {
                token->value.push_back(c);
            }
            continue;
        }
        else if (token->type == STRING && !nextToken){
            if(escapeSequence){
                switch (c) {
                    case E_INPUT_SYMBOL_LATIN_LETTERS:
                    case E_CROSS_MARK:
                        token->value.push_back(c);
                        break;
                    case 'n':
                        token->value.push_back('\n');
                        break;
                    case 't':
                        token->value.push_back('\t');
                        break;
                    case 'r':
                        token->value.push_back('\r');
                        break;
                    case 'e':
                        token->value.push_back('\e');
                        break;
                    default: {
                        ecCharToCharStack(c, tc);
                        token->position = sourcePosition;
                        compilerError(token, "Unrecognized escape sequence ❌%s.", tc);
                    }
                }
                
                escapeSequence = false;
            }
            else if (c == E_INPUT_SYMBOL_LATIN_LETTERS){
                /* End of string, time for a new token */
                nextToken = true;
            }
            else if(c == E_CROSS_MARK){
                escapeSequence = true;
            }
            else {
                detectWhitespace(c, &col, &line);
                token->value.push_back(c);
            }
            continue;
        }
        else if (token->type == VARIABLE && !nextToken){
            if(detectWhitespace(c, &col, &line)){ //
                /* End of variable */
                nextToken = true;
                continue; //do not count the whitespace again
            }
            else if(isIdentifier()){
                /* End of variable */
                nextToken = true;
            }
            else {
                token->value.push_back(c);
                continue;
            }
        }
        else if (token->type == INTEGER){
            if((47 < c && c < 58) || (((64 < c && c < 71) || (96 < c && c < 103)) && isHex)) {
                token->value.push_back(c);
                continue;
            }
            else if(c == 46) {
                /* A period. Seems to be a float */
                token->type = DOUBLE;
                token->value.push_back(c);
                continue;
            }
            else if((c == 'x' || c == 'X') && token->value.size() == 1 && token->value[0] == 48){ //Don't forget the 0
                //An X or x
                isHex = true;
                token->value.push_back(c);
                continue;
            }
            else if(c == '_'){
                continue;
            }
            else {
                //An unexcepted character, seems to be a new token
                nextToken = true;
            }
        }
        else if (token->type == DOUBLE){
            //Note: 46 is not required, we are just lexing the floating numbers
            if((47 < c && c < 58)) {
                token->value.push_back(c);
                continue;
            }
            else {
                //An unexcepted character, seems to be a new token
                nextToken = 1;
            }
        }
        else if(token->type == SYMBOL && !nextToken){
            token->value.push_back(c);
            nextToken = true;
            continue;
        }
        
        /* Do we need a new token? */
        if (nextToken){
            token = new Token(token);
            nextToken = false;
        }
        
        if (c == E_INPUT_SYMBOL_LATIN_LETTERS){
            token->type = STRING;
            token->position = sourcePosition;
        }
        else if (c == E_OLDER_WOMAN || c == E_OLDER_MAN){
            token->type = COMMENT;
            oneLineComment = (c == E_OLDER_MAN);
        }
        else if (c == E_TACO){
            token->type = DOCUMENTATION_COMMENT;
        }
        else if ((47 < c && c < 58) || c == 45 || c == 43){
            /* We've found a number or a plus or minus sign. First assume it's an integer. */
            token->type = INTEGER;
            token->position = sourcePosition;
            token->value.push_back(c);
            
            isHex = false;
        }
        else if (c == E_THUMBS_UP_SIGN || c == E_THUMBS_DOWN_SIGN){
            token->type = (c == E_THUMBS_UP_SIGN) ? BOOLEAN_TRUE : BOOLEAN_FALSE;
            token->position = sourcePosition;
            nextToken = 1;
        }
        else if (c == E_KEYCAP_10){
            token->type = SYMBOL;
            token->position = sourcePosition;
        }
        else if (detectWhitespace(c, &col, &line)){
            continue;
        }
        else if (isIdentifier()){
            token->type = IDENTIFIER;
            token->position = sourcePosition;
            token->value.push_back(c);
            nextToken = true;
        }
        else {
            token->type = VARIABLE;
            token->position = sourcePosition;
            token->value.push_back(c);
        }
    }
    delete [] stringBuffer;
    
    if (!nextToken && token->type == STRING){
        compilerError(token, "Excepted 🔤 but found end of file instead.");
    }

#undef isIdentifier
    
    return firstToken;
}
