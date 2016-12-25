//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include "Lexer.hpp"
#include "utf8.h"
#include "CompilerErrorException.hpp"
#include "EmojiTokenization.hpp"

#define isNewline() (c == 0x0A || c == 0x2028 || c == 0x2029)

bool detectWhitespace(EmojicodeChar c, size_t *col, size_t *line) {
    if (isNewline()) {
        *col = 0;
        (*line)++;
        return true;
    }
    return isWhitespace(c);
}

inline bool ends_with(std::string const & value, std::string const & ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

TokenStream lex(std::string path) {
    auto sourcePosition = SourcePosition(1, 0, path);
    Token *token = new Token(sourcePosition);
    TokenStream stream = TokenStream(token);
    token = new Token(sourcePosition, token);

    try {
        std::ifstream f(path, std::ios_base::binary | std::ios_base::in);

        if (!ends_with(path, ".emojic")) {
            throw CompilerErrorException(SourcePosition(0, 0, path),
                                         "Emojicode files must be suffixed with .emojic: %s", path.c_str());
        }
        
        EmojicodeChar c;
        size_t i = 0;

        bool nextToken = false;
        bool oneLineComment = false;
        bool isHex = false;
        bool escapeSequence = false;
        bool foundZWJ = false;

        std::stringstream stringBuffer;
        stringBuffer << f.rdbuf();
        auto string = stringBuffer.str();

        while (i < string.size()) {
            size_t delta = i;
            c = u8_nextchar(string.c_str(), &i);
            sourcePosition.character += i - delta;
            
            if (!nextToken) {
                switch (token->type()) {
                    case TokenType::Identifier:
                        if (foundZWJ && isEmoji(c)) {
                            token->value_.push_back(c);
                            foundZWJ = false;
                            continue;
                        }
                        else if ((isEmojiModifier(c) && isEmojiModifierBase(token->value_.back())) ||
                                 (isRegionalIndicator(c) && token->value().size() == 1 &&
                                  isRegionalIndicator(token->value().front()))) {
                            token->value_.push_back(c);
                            continue;
                        }
                        else if (c == 0x200D) {
                            token->value_.push_back(c);
                            foundZWJ = true;
                            continue;
                        }
                        else if (c == 0xFE0F) {  // Emojicode ignores the Emoji modifier behind an emoji character
                            continue;
                        }
                        nextToken = true;
                        break;
                    case TokenType::Comment:
                        detectWhitespace(c, &sourcePosition.character, &sourcePosition.line);
                        if (oneLineComment) {
                            if (isNewline()) {
                                token->type_ = TokenType::NoType;
                            }
                        }
                        else if (c == E_OLDER_WOMAN) {
                            token->type_ = TokenType::NoType;
                        }
                        continue;
                    case TokenType::DocumentationComment:
                        detectWhitespace(c, &sourcePosition.character, &sourcePosition.line);
                        if (c == E_TACO) {
                            nextToken = true;
                        }
                        else {
                            token->value_.push_back(c);
                        }
                        continue;
                    case TokenType::String:
                        if (escapeSequence) {
                            switch (c) {
                                case E_INPUT_SYMBOL_LATIN_LETTERS:
                                case E_CROSS_MARK:
                                    token->value_.push_back(c);
                                    break;
                                case 'n':
                                    token->value_.push_back('\n');
                                    break;
                                case 't':
                                    token->value_.push_back('\t');
                                    break;
                                case 'r':
                                    token->value_.push_back('\r');
                                    break;
                                case 'e':
                                    token->value_.push_back('\e');
                                    break;
                                default: {
                                    char tc[5] = {0, 0, 0, 0, 0};
                                    u8_wc_toutf8(tc, c);
                                    throw CompilerErrorException(sourcePosition, "Unrecognized escape sequence ❌%s.", tc);
                                }
                            }
                            
                            escapeSequence = false;
                        }
                        else if (c == E_CROSS_MARK) {
                            escapeSequence = true;
                        }
                        else if (c == E_INPUT_SYMBOL_LATIN_LETTERS) {
                            nextToken = true;
                        }
                        else {
                            detectWhitespace(c, &sourcePosition.character, &sourcePosition.line);
                            token->value_.push_back(c);
                        }
                        continue;
                    case TokenType::Variable:
                        // A variable can consist of everything except for whitespaces and identifiers (that is emojis)
                        if (detectWhitespace(c, &sourcePosition.character, &sourcePosition.line) || isEmoji(c)) {
                            nextToken = true;
                        }
                        else {
                            token->value_.push_back(c);
                            continue;
                        }
                        break;
                    case TokenType::Integer:
                        if (('0' <= c && c <= '9') || (((64 < c && c < 71) || (96 < c && c < 103)) && isHex)) {
                            token->value_.push_back(c);
                            continue;
                        }
                        else if (c == '.') {
                            token->type_ = TokenType::Double;
                            token->value_.push_back(c);
                            continue;
                        }
                        else if ((c == 'x' || c == 'X') && token->value_.size() == 1 && token->value_[0] == '0') {
                            isHex = true;
                            token->value_.push_back(c);
                            continue;
                        }
                        else if (c == '_') {
                            continue;
                        }
                        else {
                            nextToken = true;
                        }
                        break;
                    case TokenType::Double:
                        if ('0' <= c && c <= '9') {
                            token->value_.push_back(c);
                            continue;
                        }
                        else {
                            nextToken = true;
                        }
                        break;
                    case TokenType::Symbol:
                        token->value_.push_back(c);
                        nextToken = true;
                        continue;
                    default:
                        break;
                }
            }
            
            if (detectWhitespace(c, &sourcePosition.character, &sourcePosition.line)) {
                continue;
            }
            if (nextToken) {
                token->validate();
                token = new Token(sourcePosition, token);
                nextToken = false;
            }
            
            if (c == E_INPUT_SYMBOL_LATIN_LETTERS) {
                token->type_ = TokenType::String;
            }
            else if (c == E_OLDER_WOMAN || c == E_OLDER_MAN) {
                token->type_ = TokenType::Comment;
                oneLineComment = (c == E_OLDER_MAN);
            }
            else if (c == E_TACO) {
                token->type_ = TokenType::DocumentationComment;
            }
            else if (('0' <= c && c <= '9') || c == '-' || c == '+') {
                token->type_ = TokenType::Integer;
                token->value_.push_back(c);
                
                isHex = false;
            }
            else if (c == E_THUMBS_UP_SIGN || c == E_THUMBS_DOWN_SIGN) {
                token->type_ = (c == E_THUMBS_UP_SIGN) ? TokenType::BooleanTrue : TokenType::BooleanFalse;
                nextToken = true;
            }
            else if (c == E_KEYCAP_10) {
                token->type_ = TokenType::Symbol;
            }
            else if (c == 0x3016) {  // 〖
                token->type_ = TokenType::ArgumentBracketOpen;
                nextToken = true;
            }
            else if (c == 0x3017) {  // 〗
                token->type_ = TokenType::ArgumentBracketClose;
                nextToken = true;
            }
            else if (isEmoji(c)) {
                token->type_ = TokenType::Identifier;
                token->value_.push_back(c);
            }
            else {
                token->type_ = TokenType::Variable;
                token->value_.push_back(c);
            }
        }
        
        if (!nextToken && token->type() == TokenType::String) {
            throw CompilerErrorException(token->position(), "Expected 🔤 but found end of file instead.");
        }
        if (!nextToken && token->type() == TokenType::Comment && !oneLineComment) {
            throw CompilerErrorException(token->position(), "Expected 👵 but found end of file instead.");
        }
        token->validate();
    }
    catch (const std::ifstream::failure &e) {
        throw CompilerErrorException(SourcePosition(0, 0, path), "Couldn't read input file %s.", path.c_str());
    }

    return stream;
}
