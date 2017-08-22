//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Lexer.hpp"
#include "../../utf8.h"
#include "../CompilerError.hpp"
#include "EmojiTokenization.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace EmojicodeCompiler {

#define isNewline() (c == 0x0A || c == 0x2028 || c == 0x2029)

bool Lexer::detectWhitespace(EmojicodeChar c) {
    if (isNewline()) {
        sourcePosition_.character = 0;
        sourcePosition_.line++;
        return true;
    }
    return isWhitespace(c);
}

inline bool endsWith(const std::string &value, const std::string &ending) {
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

TokenStream Lexer::lexFile(const std::string &path) {
    if (!endsWith(path, ".emojic")) {
        throw CompilerError(SourcePosition(0, 0, path),
                            "Emojicode files must be suffixed with .emojic: %s", path.c_str());
    }

    std::ifstream f(path, std::ios_base::binary | std::ios_base::in);
    if (f.fail()) {
        throw CompilerError(SourcePosition(0, 0, path), "Couldn't read input file %s.", path.c_str());
    }

    std::stringstream stringBuffer;
    stringBuffer << f.rdbuf();
    return Lexer(stringBuffer.str(), path).lex();
}

void Lexer::nextChar() {
    if (!hasMoreChars()) {
        throw CompilerError(tokens_.back().position(), "Unexpected end of file.");
    }
    size_t delta = i_;
    c = u8_nextchar(string_.c_str(), &i_);
    sourcePosition_.character += i_ - delta;
}

void Lexer::nextCharOrEnd() {
    if (hasMoreChars()) {
        nextChar();
    }
    else {
        continue_ = false;
    }
}

TokenStream Lexer::lex() {
    nextCharOrEnd();
    while (continue_) {
        if (detectWhitespace(c)) {
            nextCharOrEnd();
            continue;
        }

        tokens_.emplace_back(sourcePosition_);

        TokenState state = beginToken(c, &tokens_.back()) ? TokenState::Continues : TokenState::Ended;
        while (true) {
            if (state == TokenState::NextBegun) {
                tokens_.back().validate();
                break;
            }
            if (state == TokenState::Ended) {
                tokens_.back().validate();
                nextCharOrEnd();
                break;
            }
            if (state == TokenState::Discard) {
                tokens_.pop_back();
                nextCharOrEnd();
                break;
            }
            nextChar();
            state = continueToken(c, &tokens_.back());
        }
    }

    return TokenStream(std::move(tokens_));
}

void Lexer::singleToken(Token *token, TokenType type, EmojicodeChar c) {
    token->type_ = type;
    token->value_.push_back(c);
}

bool Lexer::beginToken(EmojicodeChar c, Token *token) {
    switch (c) {
        case E_INPUT_SYMBOL_LATIN_LETTERS:
            token->type_ = TokenType::String;
            return true;
        case E_OLDER_WOMAN:
            token->type_ = TokenType::MultilineComment;
            return true;
        case E_OLDER_MAN:
            token->type_ = TokenType::SinglelineComment;
            return true;
        case E_TACO:
            token->type_ = TokenType::DocumentationComment;
            return true;
        case E_THUMBS_UP_SIGN:
        case E_THUMBS_DOWN_SIGN:
            token->type_ = (c == E_THUMBS_UP_SIGN) ? TokenType::BooleanTrue : TokenType::BooleanFalse;
            return false;
        case E_KEYCAP_10:
            token->type_ = TokenType::Symbol;
            return true;
        case E_HEAVY_PLUS_SIGN:
        case E_HEAVY_MINUS_SIGN:
        case E_HEAVY_DIVISION_SIGN:
        case E_HEAVY_MULTIPLICATION_SIGN:
        case E_OPEN_HANDS:
        case E_HANDSHAKE:
        case E_LEFT_POINTING_TRIANGLE:
        case E_RIGHT_POINTING_TRIANGLE:
        case E_LEFTWARDS_ARROW:
        case E_RIGHTWARDS_ARROW:
        case E_HEAVY_LARGE_CIRCLE:
        case E_ANGER_SYMBOL:
        case E_CROSS_MARK:
        case E_LEFT_POINTING_BACKHAND_INDEX:
        case E_RIGHT_POINTING_BACKHAND_INDEX:
        case E_PUT_LITTER_IN_ITS_SPACE:
        case E_HANDS_RAISED_IN_CELEBRATION:
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE:
        case E_RED_EXCLAMATION_MARK_AND_QUESTION_MARK:
            singleToken(token, TokenType::Operator, c);
            return false;
        case E_WHITE_EXCLAMATION_MARK:
            singleToken(token, TokenType::BeginArgumentList, c);
            return false;
        case E_RED_EXCLAMATION_MARK:
            singleToken(token, TokenType::EndArgumentList, c);
            return false;
    }

    if (('0' <= c && c <= '9') || c == '-' || c == '+') {
        token->type_ = TokenType::Integer;
        token->value_.push_back(c);

        isHex_ = false;
    }
    else if (isEmoji(c)) {
        token->type_ = TokenType::Identifier;
        token->value_.push_back(c);
    }
    else {
        token->type_ = TokenType::Variable;
        token->value_.push_back(c);
    }
    return true;
}

Lexer::TokenState Lexer::continueToken(EmojicodeChar c, Token *token) {
    switch (token->type()) {
        case TokenType::Identifier:
            if (foundZWJ_ && isEmoji(c)) {
                token->value_.push_back(c);
                foundZWJ_ = false;
                return TokenState::Continues;
            }
            if ((isEmojiModifier(c) && isEmojiModifierBase(token->value_.back())) ||
                (isRegionalIndicator(c) && token->value().size() == 1 && isRegionalIndicator(token->value().front()))) {
                 token->value_.push_back(c);
                 return TokenState::Continues;
             }
            if (c == 0x200D) {
                token->value_.push_back(c);
                foundZWJ_ = true;
                return TokenState::Continues;
            }
            if (c == 0xFE0F) {  // Emojicode ignores the Emoji modifier behind an emoji character
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::SinglelineComment:
            detectWhitespace(c);
            if (isNewline()) {
                return TokenState::Discard;
            }
            return TokenState::Continues;
        case TokenType::MultilineComment:
            if (c == E_OLDER_WOMAN) {
                return TokenState::Discard;
            }
            return TokenState::Continues;
        case TokenType::DocumentationComment:
            detectWhitespace(c);
            if (c == E_TACO) {
                return TokenState::Ended;
            }
            token->value_.push_back(c);
            return TokenState::Continues;
        case TokenType::String:
            if (escapeSequence_) {
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
                    default: {
                        char tc[5] = {0, 0, 0, 0, 0};
                        u8_wc_toutf8(tc, c);
                        throw CompilerError(sourcePosition_, "Unrecognized escape sequence ❌%s.", tc);
                    }
                }

                escapeSequence_ = false;
            }
            else if (c == E_CROSS_MARK) {
                escapeSequence_ = true;
            }
            else if (c == E_INPUT_SYMBOL_LATIN_LETTERS) {
                return TokenState::Ended;
            }

            detectWhitespace(c);
            token->value_.push_back(c);
            return TokenState::Continues;
        case TokenType::Variable:
            // A variable can consist of everything except for whitespaces and identifiers (that is emojis)
            // isWhitespace used here because if it is whitespace, the detection will take place below
            if (isWhitespace(c) || isEmoji(c)) {
                return TokenState::NextBegun;
            }

            token->value_.push_back(c);
            return TokenState::Continues;
        case TokenType::Integer:
            if (('0' <= c && c <= '9') || (((64 < c && c < 71) || (96 < c && c < 103)) && isHex_)) {
                token->value_.push_back(c);
                return TokenState::Continues;
            }
            else if (c == '.') {
                token->type_ = TokenType::Double;
                token->value_.push_back(c);
                return TokenState::Continues;
            }
            else if ((c == 'x' || c == 'X') && token->value_.size() == 1 && token->value_[0] == '0') {
                isHex_ = true;
                token->value_.push_back(c);
                return TokenState::Continues;
            }
            else if (c == '_') {
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::Double:
            if ('0' <= c && c <= '9') {
                token->value_.push_back(c);
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::Symbol:
            token->value_.push_back(c);
            return TokenState::Ended;
        default:
            throw std::logic_error("Lexer: Token continued but not handled.");
    }
}

}  // namespace EmojicodeCompiler
