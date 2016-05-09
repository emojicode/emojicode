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
        case BOOLEAN_FALSE:
            return "Boolean False";
        case BOOLEAN_TRUE:
            return "Boolean True";
        case DOUBLE:
            return "Double";
        case INTEGER:
            return "Integer";
        case STRING:
            return "String";
        case SYMBOL:
            return "Symbol";
        case VARIABLE:
            return "Variable";
        case IDENTIFIER:
            return "Identifier";
        case DOCUMENTATION_COMMENT:
            return "Documentation Comment";
        case ARGUMENT_BRACKET_CLOSE:
            return "Argument Bracket Close";
        case ARGUMENT_BRACKET_OPEN:
            return "Argument Bracket Open";
        case NO_TYPE:
        case COMMENT:
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
