//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Lexer.hpp"
#include "../CompilerError.hpp"
#include "EmojiTokenization.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <locale>
#include <codecvt>

namespace EmojicodeCompiler {

bool Lexer::detectWhitespace() {
    if (isNewline()) {
        sourcePosition_.character = 0;
        sourcePosition_.line++;
        return true;
    }
    return isWhitespace(codePoint());
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

    auto string = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return Lexer(conv.from_bytes(string), path).lex();
}

void Lexer::nextChar() {
    if (!hasMoreChars()) {
        throw CompilerError(tokens_.back().position(), "Unexpected end of file.");
    }
    codePoint_ = string_[i_++];
    sourcePosition_.character++;
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
        if (detectWhitespace()) {
            nextCharOrEnd();
            continue;
        }

        tokens_.emplace_back(sourcePosition_);

        readToken(&tokens_.back());
    }

    return TokenStream(std::move(tokens_));
}

void Lexer::readToken(Token *token) {
    TokenState state = beginToken(token) ? TokenState::Continues : TokenState::Ended;
    while (true) {
        if (state == TokenState::NextBegun) {
            token->validate();
            return;
        }
        if (state == TokenState::Ended) {
            token->validate();
            nextCharOrEnd();
            return;
        }
        if (state == TokenState::Discard) {
            tokens_.pop_back();
            nextCharOrEnd();
            return;
        }
        nextChar();
        detectWhitespace();
        state = continueToken(token);
    }
}

void Lexer::singleToken(Token *token, TokenType type, char32_t c) {
    token->type_ = type;
    token->value_.push_back(c);
}

bool Lexer::beginToken(Token *token) {
    switch (codePoint()) {
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
            token->type_ = (codePoint() == E_THUMBS_UP_SIGN) ? TokenType::BooleanTrue : TokenType::BooleanFalse;
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
            singleToken(token, TokenType::Operator, codePoint());
            return false;
        case E_WHITE_EXCLAMATION_MARK:
            singleToken(token, TokenType::BeginArgumentList, codePoint());
            return false;
        case E_RED_EXCLAMATION_MARK:
            singleToken(token, TokenType::EndArgumentList, codePoint());
            return false;
    }

    if (('0' <= codePoint() && codePoint() <= '9') || codePoint() == '-' || codePoint() == '+') {
        token->type_ = TokenType::Integer;
        token->value_.push_back(codePoint());

        isHex_ = false;
    }
    else if (isEmoji(codePoint())) {
        token->type_ = TokenType::Identifier;
        token->value_.push_back(codePoint());
    }
    else {
        token->type_ = TokenType::Variable;
        token->value_.push_back(codePoint());
    }
    return true;
}

Lexer::TokenState Lexer::continueToken(Token *token) {
    switch (token->type()) {
        case TokenType::Identifier:
            if (foundZWJ_ && isEmoji(codePoint())) {
                token->value_.push_back(codePoint());
                foundZWJ_ = false;
                return TokenState::Continues;
            }
            if ((isEmojiModifier(codePoint()) && isEmojiModifierBase(token->value_.back())) ||
                (isRegionalIndicator(codePoint()) && token->value().size() == 1 && isRegionalIndicator(token->value().front()))) {
                 token->value_.push_back(codePoint());
                 return TokenState::Continues;
             }
            if (codePoint() == 0x200D) {
                token->value_.push_back(codePoint());
                foundZWJ_ = true;
                return TokenState::Continues;
            }
            if (codePoint() == 0xFE0F) {  // Emojicode ignores the Emoji modifier behind an emoji character
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::SinglelineComment:
            if (isNewline()) {
                return TokenState::Discard;
            }
            return TokenState::Continues;
        case TokenType::MultilineComment:
            if (codePoint() == E_OLDER_WOMAN) {
                return TokenState::Discard;
            }
            return TokenState::Continues;
        case TokenType::DocumentationComment:
            if (codePoint() == E_TACO) {
                return TokenState::Ended;
            }
            token->value_.push_back(codePoint());
            return TokenState::Continues;
        case TokenType::String:
            if (escapeSequence_) {
                switch (codePoint()) {
                    case E_INPUT_SYMBOL_LATIN_LETTERS:
                    case E_CROSS_MARK:
                        token->value_.push_back(codePoint());
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
                        throw CompilerError(sourcePosition_, "Unrecognized escape sequence ❌%s.",
                                            utf8(std::u32string(1, codePoint())).c_str());
                    }
                }

                escapeSequence_ = false;
            }
            else if (codePoint() == E_CROSS_MARK) {
                escapeSequence_ = true;
            }
            else if (codePoint() == E_INPUT_SYMBOL_LATIN_LETTERS) {
                return TokenState::Ended;
            }

            token->value_.push_back(codePoint());
            return TokenState::Continues;
        case TokenType::Variable:
            // A variable can consist of everything except for whitespaces and identifiers (that is emojis)
            // isWhitespace used here because if it is whitespace, the detection will take place below
            if (isWhitespace(codePoint()) || isEmoji(codePoint())) {
                return TokenState::NextBegun;
            }

            token->value_.push_back(codePoint());
            return TokenState::Continues;
        case TokenType::Integer:
            if (('0' <= codePoint() && codePoint() <= '9') || (((64 < codePoint() && codePoint() < 71) || (96 < codePoint() && codePoint() < 103)) && isHex_)) {
                token->value_.push_back(codePoint());
                return TokenState::Continues;
            }
            else if (codePoint() == '.') {
                token->type_ = TokenType::Double;
                token->value_.push_back(codePoint());
                return TokenState::Continues;
            }
            else if ((codePoint() == 'x' || codePoint() == 'X') && token->value_.size() == 1 && token->value_[0] == '0') {
                isHex_ = true;
                token->value_.push_back(codePoint());
                return TokenState::Continues;
            }
            else if (codePoint() == '_') {
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::Double:
            if ('0' <= codePoint() && codePoint() <= '9') {
                token->value_.push_back(codePoint());
                return TokenState::Continues;
            }
            return TokenState::NextBegun;
        case TokenType::Symbol:
            token->value_.push_back(codePoint());
            return TokenState::Ended;
        default:
            throw std::logic_error("Lexer: Token continued but not handled.");
    }
}

}  // namespace EmojicodeCompiler
