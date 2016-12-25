//
//  Token.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Token.hpp"
#include "CompilerErrorException.hpp"
#include "EmojiTokenization.hpp"

SourcePosition::SourcePosition(const Token &token) : SourcePosition(token.position()) {
    
}

const char* Token::stringName() const {
    return stringNameForType(type());
}

const char* Token::stringNameForType(TokenType type) {
    switch (type) {
        case TokenType::BooleanFalse:
            return "Boolean False";
        case TokenType::BooleanTrue:
            return "Boolean True";
        case TokenType::Double:
            return "Double";
        case TokenType::Integer:
            return "Integer";
        case TokenType::String:
            return "String";
        case TokenType::Symbol:
            return "Symbol";
        case TokenType::Variable:
            return "Variable";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::DocumentationComment:
            return "Documentation Comment";
        case TokenType::ArgumentBracketClose:
            return "Argument Bracket Close";
        case TokenType::ArgumentBracketOpen:
            return "Argument Bracket Open";
        case TokenType::NoType:
        case TokenType::Comment:
            break;
    }
    return "Mysterious unnamed token";
}

void Token::validate() const {
    switch (type()) {
        case TokenType::Integer:
            if (value().back() == 'x') {
                throw CompilerErrorException(position(), "Expected a digit after integer literal prefix.");
            }
            break;
        case TokenType::Double:
            if (value().back() == '.') {
                throw CompilerErrorException(position(), "Expected a digit after decimal seperator.");
            }
            break;
        case TokenType::Identifier:
            if (!isValidEmoji(value())) {
                throw CompilerErrorException(position(), "Invalid emoji.");
            }
        default:
            break;
    }
}

bool Token::isIdentifier(EmojicodeChar ch) const {
    return type() == TokenType::Identifier && value_.size() == 1 && value_[0] == ch;
}
