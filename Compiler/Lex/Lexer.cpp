//
//  Lexer.c
//  Emojicode
//
//  Created by Theo Weidmann on 28.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Lexer.hpp"
#include "CompilerError.hpp"
#include "EmojiTokenization.hpp"
#include "Emojis.h"

namespace EmojicodeCompiler {

Lexer::Lexer(SourceFile *source, bool minimalMode)
        : sourcePosition_(1, 0, source), source_(source), minimalMode_(minimalMode) {
    skipWhitespace();

    loadOperatorSingleTokens();
    singleTokens_.emplace(E_RED_EXCLAMATION_MARK, TokenType::EndArgumentList);
    singleTokens_.emplace(E_RED_QUESTION_MARK, TokenType::EndInterrogativeArgumentList);
    singleTokens_.emplace(E_RIGHT_FACING_FIST, TokenType::GroupBegin);
    singleTokens_.emplace(E_LEFT_FACING_FIST, TokenType::GroupEnd);
    singleTokens_.emplace(E_RIGHT_ARROW_CURVING_LEFT, TokenType::Return);
    singleTokens_.emplace(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS, TokenType::RepeatWhile);
    singleTokens_.emplace(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY,
                          TokenType::ForIn);
    singleTokens_.emplace(E_THUMBS_UP_SIGN, TokenType::BooleanTrue);
    singleTokens_.emplace(E_THUMBS_DOWN_SIGN, TokenType::BooleanFalse);
    singleTokens_.emplace(E_POLICE_CARS_LIGHT, TokenType::Error);
    singleTokens_.emplace(E_LEFT_ARROW_CURVING_RIGHT, TokenType::If);
    singleTokens_.emplace(E_OK, TokenType::ErrorHandler);
    singleTokens_.emplace(E_GRAPES, TokenType::BlockBegin);
    singleTokens_.emplace(E_WATERMELON, TokenType::BlockEnd);
    singleTokens_.emplace(E_NEW_SIGN, TokenType::New);
    singleTokens_.emplace(E_HAND_POINTING_DOWN, TokenType::This);
    singleTokens_.emplace(E_BIOHAZARD, TokenType::Unsafe);
    singleTokens_.emplace(E_RIGHT_ARROW_CURVING_UP, TokenType::Super);
    singleTokens_.emplace(E_RIGHTWARDS_ARROW, TokenType::RightProductionOperator);
    singleTokens_.emplace(E_LEFTWARDS_ARROW, TokenType::LeftProductionOperator);
    singleTokens_.emplace(E_CRAYON, TokenType::Mutable);
    singleTokens_.emplace(E_SPIRAL_SHELL, TokenType::Generic);

    singleTokens_.emplace(E_CROCODILE, TokenType::Protocol);
    singleTokens_.emplace(E_DOVE_OF_PEACE, TokenType::ValueType);
    singleTokens_.emplace(E_RABBIT, TokenType::Class);
    singleTokens_.emplace(E_RADIO_BUTTON, TokenType::Enumeration);
    singleTokens_.emplace(E_CHEERING_MEGAPHONE, TokenType::SelectionOperator);
}

void Lexer::loadOperatorSingleTokens() {
    singleTokens_.emplace(E_HEAVY_PLUS_SIGN, TokenType::Operator);
    singleTokens_.emplace(E_HEAVY_MINUS_SIGN, TokenType::Operator);
    singleTokens_.emplace(E_HEAVY_DIVISION_SIGN, TokenType::Operator);
    singleTokens_.emplace(E_HEAVY_MULTIPLICATION_SIGN, TokenType::Operator);
    singleTokens_.emplace(E_OPEN_HANDS, TokenType::Operator);
    singleTokens_.emplace(E_HANDSHAKE, TokenType::Operator);
    singleTokens_.emplace(E_HEAVY_LARGE_CIRCLE, TokenType::Operator);
    singleTokens_.emplace(E_ANGER_SYMBOL, TokenType::Operator);
    singleTokens_.emplace(E_CROSS_MARK, TokenType::Operator);
    singleTokens_.emplace(E_LEFT_POINTING_BACKHAND_INDEX, TokenType::Operator);
    singleTokens_.emplace(E_RIGHT_POINTING_BACKHAND_INDEX, TokenType::Operator);
    singleTokens_.emplace(E_PUT_LITTER_IN_ITS_SPACE, TokenType::Operator);
    singleTokens_.emplace(E_HANDS_RAISED_IN_CELEBRATION, TokenType::Operator);
    singleTokens_.emplace(E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE, TokenType::Operator);
    singleTokens_.emplace(E_RED_EXCLAMATION_MARK_AND_QUESTION_MARK, TokenType::Call);
}

void Lexer::skipWhitespace() {
    while (continue_ && detectWhitespace()) {
        nextCharOrEnd();
    }
}

bool Lexer::detectWhitespace() {
    if (isNewline()) {
        sourcePosition_.character = 0;
        sourcePosition_.line++;
        if (!minimalMode_) {
            source_->endLine(i_ + 1);
        }
        return false;
    }
    return isWhitespace();
}

void Lexer::nextChar() {
    if (!hasMoreChars()) {
        throw CompilerError(sourcePosition_, "Unexpected end of file.");
    }
    i_++;
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

Token Lexer::lex() {
    Token token = readToken();
    skipWhitespace();
    if (minimalMode_ && (token.type() == TokenType::MultilineComment || token.type() == TokenType::SinglelineComment)) {
        return lex();
    }
    return token;
}

Token Lexer::readToken() {
    Token token(sourcePosition_);
    TokenConstructionState constState;

    TokenState state = beginToken(&token, &constState) ? TokenState::Continues : TokenState::Ended;
    while (true) {
        if (state == TokenState::Ended) {
            token.validate();
            nextCharOrEnd();
            return token;
        }
        nextChar();
        state = continueToken(&token, &constState);
        if (state == TokenState::NextBegun) {
            token.validate();
            return token;
        }
        // Whitespace must be detected here so that this method returns on NextBegun without calling detectWhitespace()
        // as the detectWhitespace() would otherwise be called twice for the same character. (Here and in lex())
        detectWhitespace();
    }
}

bool Lexer::beginToken(Token *token, TokenConstructionState *constState) const {
    auto it = singleTokens_.find(codePoint());
    if (it != singleTokens_.end()) {
        token->type_ = it->second;
        token->value_.push_back(codePoint());
        return false;
    }

    switch (codePoint()) {
        case E_INPUT_SYMBOL_LATIN_LETTERS:
            token->type_ = TokenType::String;
            return true;
        case E_THOUGHT_BALLOON:
            token->type_ = TokenType::SinglelineComment;
            return true;
        case E_GREEN_TEXTBOOK:
            token->type_ = TokenType::DocumentationComment;
            return true;
        case E_BLUE_TEXTBOOK:
            token->type_ = TokenType::PackageDocumentationComment;
            return true;
        case E_KEYCAP_10:
            token->type_ = TokenType::Symbol;
            return true;
        case E_LEFT_POINTING_TRIANGLE:
        case E_RIGHT_POINTING_TRIANGLE:
            token->type_ = TokenType::Operator;
            token->value_.push_back(codePoint());
            return true;
        case E_PINE_DECORATION:
            token->type_ = TokenType::Decorator;
            return true;
        case E_MAGNET:
            token->type_ = TokenType::MiddleInterpolation;
            return true;
        default:
            break;
    }

    if (isNewline()) {
        token->type_ = TokenType::LineBreak;
        return hasMoreChars();
    }

    if (('0' <= codePoint() && codePoint() <= '9') || codePoint() == '-' || codePoint() == '+') {
        token->type_ = TokenType::Integer;
        constState->isHex_ = false;
    }
    else if (isEmoji(codePoint())) {
        token->type_ = TokenType::Identifier;
    }
    else {
        token->type_ = TokenType::Variable;
    }
    token->value_.push_back(codePoint());
    return hasMoreChars();
}

Lexer::TokenState Lexer::continueToken(Token *token, TokenConstructionState *constState) const {
    switch (token->type()) {
        case TokenType::Decorator: {
            if (!isEmoji(codePoint())) {
                throw CompilerError(position(), "🎍 must be followed by an emoji.");
            }
            token->value_.push_back(codePoint());
            return TokenState::Ended;
        }
        case TokenType::Identifier:
            return continueIdentifierToken(token, constState);
        case TokenType::Operator:
            return continueOperator(token);
        case TokenType::SinglelineComment:
            return continueSingleLineToken(token, constState);
        case TokenType::MultilineComment:
            return continueMultilineComment(token, constState);
        case TokenType::DocumentationComment:
            if (codePoint() == E_GREEN_TEXTBOOK) {
                return TokenState::Ended;
            }
            token->value_.push_back(codePoint());
            return TokenState::Continues;
        case TokenType::PackageDocumentationComment:
            if (codePoint() == E_BLUE_TEXTBOOK) {
                return TokenState::Ended;
            }
            token->value_.push_back(codePoint());
            return TokenState::Continues;
        case TokenType::String:
        case TokenType::MiddleInterpolation:
            return continueStringToken(token, constState);
        case TokenType::Variable:
            return continueVariableToken(token);
        case TokenType::Integer:
            return continueIntegerToken(token, constState);
        case TokenType::Double:
            return continueDoubleToken(token);
        case TokenType::Symbol:
            token->value_.push_back(codePoint());
            return TokenState::Ended;
        case TokenType::LineBreak:
            if (isNewline()) {
                token->type_ = TokenType::BlankLine;
                return TokenState::Ended;
            }
            return isWhitespace() ? hasMoreChars() ? TokenState::Continues : TokenState::Ended : TokenState::NextBegun;
        default:
            throw std::logic_error("Lexer: Token continued but not handled.");
    }
}

Lexer::TokenState Lexer::continueMultilineComment(Token *token, TokenConstructionState *constState) const {
    if (!constState->commentDetermined_) {
        if (codePoint() == E_THOUGHT_BALLOON) {
            token->value_.pop_back();
            return TokenState::Ended;
        }
        constState->commentDetermined_ = true;
    }
    if (codePoint() == E_END_ARROW) {
        constState->commentDetermined_ = false;
    }
    if (!minimalMode_) {
        token->value_.push_back(codePoint());
    }
    return TokenState::Continues;
}

Lexer::TokenState Lexer::continueSingleLineToken(Token *token, TokenConstructionState *constState) const {
    if (!constState->commentDetermined_) {
        if (codePoint() == E_SOON_ARROW) {
            token->type_ = TokenType::MultilineComment;
            return TokenState::Continues;
        }
        constState->commentDetermined_ = true;
    }

    if (isNewline()) {
        return TokenState::Ended;
    }
    if (!minimalMode_) {
        token->value_.push_back(codePoint());
    }
    return TokenState::Continues;
}

Lexer::TokenState Lexer::continueDoubleToken(Token *token) const {
    if ('0' <= codePoint() && codePoint() <= '9') {
        token->value_.push_back(codePoint());
        return TokenState::Continues;
    }
    return TokenState::NextBegun;
}

Lexer::TokenState Lexer::continueVariableToken(Token *token) const {
    // A variable can consist of everything except for whitespaces and identifiers (that is emojis)
    // isWhitespace used here because if it is whitespace, the detection will take place below
    if (isWhitespace() || isEmoji(codePoint())) {
        return TokenState::NextBegun;
    }

    token->value_.push_back(codePoint());
    return TokenState::Continues;
}

Lexer::TokenState Lexer::continueIntegerToken(Token *token, Lexer::TokenConstructionState *constState) const {
    if (('0' <= codePoint() && codePoint() <= '9') ||
        (((64 < codePoint() && codePoint() < 71) || (96 < codePoint() && codePoint() < 103)) &&
         constState->isHex_)) {
        token->value_.push_back(codePoint());
        return TokenState::Continues;
    }
    if (codePoint() == '.') {
        token->type_ = TokenType::Double;
        token->value_.push_back(codePoint());
        return TokenState::Continues;
    }
    if ((codePoint() == 'x' || codePoint() == 'X') && token->value_.size() == 1 && token->value_[0] == '0') {
        constState->isHex_ = true;
        token->value_.push_back(codePoint());
        return TokenState::Continues;
    }
    if (codePoint() == ',') {
        return TokenState::Continues;
    }
    return TokenState::NextBegun;
}

Lexer::TokenState Lexer::continueStringToken(Token *token, Lexer::TokenConstructionState *constState) const {
    if (constState->escapeSequence_) {
        handleEscapeSequence(token, constState);
        return TokenState::Continues;
    }
    if (codePoint() == E_CROSS_MARK) {
        constState->escapeSequence_ = true;
        return TokenState::Continues;
    }
    if (codePoint() == E_INPUT_SYMBOL_LATIN_LETTERS) {
        if (token->type_ == TokenType::MiddleInterpolation) {
            token->type_ = TokenType::EndInterpolation;
        }
        return TokenState::Ended;
    }
    if (codePoint() == E_MAGNET) {
        if (token->type_ != TokenType::MiddleInterpolation) {
            token->type_ = TokenType::BeginInterpolation;
        }
        return TokenState::Ended;
    }

    token->value_.push_back(codePoint());
    return TokenState::Continues;
}

void Lexer::handleEscapeSequence(Token *token, Lexer::TokenConstructionState *constState) const {
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
        case E_MAGNET:
            token->value_.push_back(E_MAGNET);
            break;
        default: {
            throw CompilerError(sourcePosition_, "Unrecognized escape sequence ❌",
                                utf8(std::u32string(1, codePoint())), ".");
        }
    }

    constState->escapeSequence_ = false;
}

Lexer::TokenState Lexer::continueIdentifierToken(Token *token, Lexer::TokenConstructionState *constState) const {
    if (constState->foundZWJ_ && isEmoji(codePoint())) {
        token->value_.push_back(codePoint());
        constState->foundZWJ_ = false;
        return TokenState::Continues;
    }
    if ((isEmojiModifier(codePoint()) && isEmojiModifierBase(token->value_.back())) ||
        (isRegionalIndicator(codePoint()) && token->value().size() == 1 &&
         isRegionalIndicator(token->value().front()))) {
        token->value_.push_back(codePoint());
        return TokenState::Continues;
    }
    if (codePoint() == 0x200D || codePoint() == E_SMALL_ORANGE_DIAMOND) {
        token->value_.push_back(codePoint());
        constState->foundZWJ_ = true;
        return TokenState::Continues;
    }
    if (codePoint() == 0xFE0F) {  // Emojicode ignores the Emoji modifier behind an emoji character
        return TokenState::Continues;
    }
    if (token->value_.front() == E_PERSON_SHRUGGING) {
        token->type_ = TokenType::NoValue;
    }
    if (token->value().front() == E_NO_GESTURE) {
        if (codePoint() == E_LEFT_ARROW_CURVING_RIGHT) {
            token->type_ = TokenType::ElseIf;
            return TokenState::Ended;
        }
        token->type_ = TokenType::Else;
    }
    return TokenState::NextBegun;
}

Lexer::TokenState Lexer::continueOperator(Token *token) const {
    if (codePoint() == 0xFE0F) {  // Emojicode ignores the Emoji modifier behind an emoji character
        return TokenState::Continues;
    }
    if (codePoint() == E_HANDS_RAISED_IN_CELEBRATION) {
        token->value_.push_back(codePoint());
        return TokenState::Ended;
    }
    return TokenState::NextBegun;
}

}  // namespace EmojicodeCompiler
