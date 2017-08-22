//
//  Token.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Token.hpp"
#include "../CompilerError.hpp"
#include "EmojiTokenization.hpp"

namespace EmojicodeCompiler {

const char* Token::stringName() const {
    return stringNameForType(type());
}

const char* Token::stringNameForType(TokenType type) {
    switch (type) {
        case TokenType::NoType: return "NoType";
        case TokenType::String: return "String";
        case TokenType::MultilineComment: return "MultilineComment";
        case TokenType::SinglelineComment: return "SinglelineComment";
        case TokenType::DocumentationComment: return "DocumentationComment";
        case TokenType::Integer: return "Integer";
        case TokenType::Double: return "Double";
        case TokenType::BooleanTrue: return "BooleanTrue";
        case TokenType::BooleanFalse: return "BooleanFalse";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Variable: return "Variable";
        case TokenType::Symbol: return "Symbol";
        case TokenType::Operator: return "Operator";
        case TokenType::EndArgumentList: return "EndOfArguments";
        case TokenType::BeginArgumentList: return "BeginArgumentList";
    }
}

void Token::validate() const {
    switch (type()) {
        case TokenType::Integer:
            if (value().back() == 'x') {
                throw CompilerError(position(), "Expected a digit after integer literal prefix.");
            }
            break;
        case TokenType::Double:
            if (value().back() == '.') {
                throw CompilerError(position(), "Expected a digit after decimal seperator.");
            }
            break;
        case TokenType::Identifier:
            if (!isValidEmoji(value())) {
                throw CompilerError(position(), "Invalid emoji.");
            }
        default:
            break;
    }
}

bool Token::isIdentifier(EmojicodeChar ch) const {
    return type() == TokenType::Identifier && value_.size() == 1 && value_[0] == ch;
}

}  // namespace EmojicodeCompiler
