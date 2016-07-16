//
//  Token.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Token.hpp"
#include "CompilerErrorException.hpp"

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

void Token::validateInteger(bool isHex) const {
    if (isHex && value.size() < 3) {
        throw CompilerErrorException(position(), "Expected a digit after integer literal prefix.");
    }
}

void Token::validateDouble() const {
    if (value.back() == '.') {
        throw CompilerErrorException(position(), "Expected a digit after decimal seperator.");
    }
}
