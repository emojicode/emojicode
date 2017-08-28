//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Lexer.hpp"
#include "utf8.h"
#include "CompilerError.hpp"
#include "EmojiTokenization.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace EmojicodeCompiler {

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
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

TokenStream lex(const std::string &path) {
    if (!ends_with(path, ".emojic")) {
        throw CompilerError(SourcePosition(0, 0, path),
                            "Emojicode files must be suffixed with .emojic: %s", path.c_str());
    }

    std::ifstream f(path, std::ios_base::binary | std::ios_base::in);
    if (f.fail()) {
        throw CompilerError(SourcePosition(0, 0, path), "Couldn't read input file %s.", path.c_str());
    }

    std::stringstream stringBuffer;
    stringBuffer << f.rdbuf();
    auto string = stringBuffer.str();

    auto sourcePosition = SourcePosition(1, 0, path);
    return lexString(string, sourcePosition);
}

TokenStream lexString(const std::string &string, SourcePosition sourcePosition) {
    EmojicodeChar c;
    size_t i = 0;

    bool nextToken = false;
    bool oneLineComment = false;
    bool isHex = false;
    bool escapeSequence = false;
    bool foundZWJ = false;

    auto tokens = std::make_shared<std::vector<Token>>();
    tokens->emplace_back(sourcePosition);

    while (i < string.size()) {
        size_t delta = i;
        c = u8_nextchar(string.c_str(), &i);
        sourcePosition.character += i - delta;

        if (!nextToken) {
            switch (tokens->back().type()) {
                case TokenType::Identifier:
                    if (foundZWJ && isEmoji(c)) {
                        tokens->back().value_.push_back(c);
                        foundZWJ = false;
                        continue;
                    }
                    else if ((isEmojiModifier(c) && isEmojiModifierBase(tokens->back().value_.back())) ||
                             (isRegionalIndicator(c) && tokens->back().value().size() == 1 &&
                              isRegionalIndicator(tokens->back().value().front()))) {
                                 tokens->back().value_.push_back(c);
                                 continue;
                             }
                    else if (c == 0x200D) {
                        tokens->back().value_.push_back(c);
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
                            tokens->back().type_ = TokenType::NoType;
                        }
                    }
                    else if (c == E_OLDER_WOMAN) {
                        tokens->back().type_ = TokenType::NoType;
                    }
                    continue;
                case TokenType::DocumentationComment:
                    detectWhitespace(c, &sourcePosition.character, &sourcePosition.line);
                    if (c == E_TACO) {
                        nextToken = true;
                    }
                    else {
                        tokens->back().value_.push_back(c);
                    }
                    continue;
                case TokenType::String:
                    if (escapeSequence) {
                        switch (c) {
                            case E_INPUT_SYMBOL_LATIN_LETTERS:
                            case E_CROSS_MARK:
                                tokens->back().value_.push_back(c);
                                break;
                            case 'n':
                                tokens->back().value_.push_back('\n');
                                break;
                            case 't':
                                tokens->back().value_.push_back('\t');
                                break;
                            case 'r':
                                tokens->back().value_.push_back('\r');
                                break;
                            default: {
                                char tc[5] = {0, 0, 0, 0, 0};
                                u8_wc_toutf8(tc, c);
                                throw CompilerError(sourcePosition, "Unrecognized escape sequence ❌%s.",
                                                             tc);
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
                        tokens->back().value_.push_back(c);
                    }
                    continue;
                case TokenType::Variable:
                    // A variable can consist of everything except for whitespaces and identifiers (that is emojis)
                    // isWhitespace used here because if it is whitespace, the detection will take place below
                    if (isWhitespace(c) || isEmoji(c)) {
                        nextToken = true;
                    }
                    else {
                        tokens->back().value_.push_back(c);
                        continue;
                    }
                    break;
                case TokenType::Integer:
                    if (('0' <= c && c <= '9') || (((64 < c && c < 71) || (96 < c && c < 103)) && isHex)) {
                        tokens->back().value_.push_back(c);
                        continue;
                    }
                    else if (c == '.') {
                        tokens->back().type_ = TokenType::Double;
                        tokens->back().value_.push_back(c);
                        continue;
                    }
                    else if ((c == 'x' || c == 'X') && tokens->back().value_.size() == 1 && tokens->back().value_[0] == '0') {
                        isHex = true;
                        tokens->back().value_.push_back(c);
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
                        tokens->back().value_.push_back(c);
                        continue;
                    }
                    else {
                        nextToken = true;
                    }
                    break;
                case TokenType::Symbol:
                    tokens->back().value_.push_back(c);
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
            tokens->back().validate();
            tokens->emplace_back(sourcePosition);
            nextToken = false;
        }

        if (c == E_INPUT_SYMBOL_LATIN_LETTERS) {
            tokens->back().type_ = TokenType::String;
        }
        else if (c == E_OLDER_WOMAN || c == E_OLDER_MAN) {
            tokens->back().type_ = TokenType::Comment;
            oneLineComment = (c == E_OLDER_MAN);
        }
        else if (c == E_TACO) {
            tokens->back().type_ = TokenType::DocumentationComment;
        }
        else if (('0' <= c && c <= '9') || c == '-' || c == '+') {
            tokens->back().type_ = TokenType::Integer;
            tokens->back().value_.push_back(c);

            isHex = false;
        }
        else if (c == E_THUMBS_UP_SIGN || c == E_THUMBS_DOWN_SIGN) {
            tokens->back().type_ = (c == E_THUMBS_UP_SIGN) ? TokenType::BooleanTrue : TokenType::BooleanFalse;
            nextToken = true;
        }
        else if (c == E_KEYCAP_10) {
            tokens->back().type_ = TokenType::Symbol;
        }
        else if (isEmoji(c)) {
            tokens->back().type_ = TokenType::Identifier;
            tokens->back().value_.push_back(c);
        }
        else {
            tokens->back().type_ = TokenType::Variable;
            tokens->back().value_.push_back(c);
        }
    }

    if (!nextToken && tokens->back().type() == TokenType::String) {
        throw CompilerError(tokens->back().position(), "Expected 🔤 but found end of file instead.");
    }
    if (!nextToken && tokens->back().type() == TokenType::Comment && !oneLineComment) {
        throw CompilerError(tokens->back().position(), "Expected 👵 but found end of file instead.");
    }
    tokens->back().validate();

    return TokenStream(tokens);
}

}  // namespace EmojicodeCompiler
