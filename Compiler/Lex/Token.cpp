//
//  Token.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Token.hpp"
#include "CompilerError.hpp"
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
        case TokenType::GroupEnd: return "GroupEnd";
        case TokenType::GroupBegin: return "GroupBegin";
        case TokenType::Return: return "Return";
        case TokenType::RepeatWhile: return "RepeatWhile";
        case TokenType::ForIn: return "ForIn";
        case TokenType::Error: return "Error";
        case TokenType::If: return "If";
        case TokenType::ElseIf: return "ElseIf";
        case TokenType::Else: return "Else";
        case TokenType::SuperInit: return "SuperInit";
        case TokenType::FrozenDeclaration: return "FrozenDeclaration";
        case TokenType::Declaration: return "Declaration";
        case TokenType::Assignment: return "Assignment";
        case TokenType::ErrorHandler: return "ErrorHandler";
        case TokenType::BlockBegin: return "BlockBegin";
        case TokenType::BlockEnd: return "BlockEnd";
        case TokenType::New: return "New";
        case TokenType::This: return "This";
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

bool Token::isIdentifier(char32_t ch) const {
    return type() == TokenType::Identifier && value_.size() == 1 && value_[0] == ch;
}

}  // namespace EmojicodeCompiler
