//
//  Token.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Token_hpp
#define Token_hpp

#include "SourcePosition.hpp"
#include <string>
#include <utility>

namespace EmojicodeCompiler {

class Lexer;

enum class TokenType {
    NoType,
    String,
    MultilineComment,
    SinglelineComment,
    DocumentationComment,
    Integer,
    Double,
    BooleanTrue,
    BooleanFalse,
    Identifier,
    Variable,
    Symbol,
    Operator,
    BeginArgumentList,
    EndArgumentList,
    BeginInterrogativeArgumentList,
    EndInterrogativeArgumentList,
    GroupBegin,
    GroupEnd,
    RightProductionOperator,

    BlockBegin,
    BlockEnd,
    Return,
    RepeatWhile,
    ForIn,
    Error,
    If,
    ElseIf,
    Else,
    FrozenDeclaration,
    Declaration,
    Assignment,
    ErrorHandler,
    New,
    This,
    Unsafe,
    NoValue,
    Super,
};

class Token {
    friend Lexer;
public:
    explicit Token(SourcePosition p) : position_(std::move(p)) {}

    /** Returns the position at which this token was defined. */
    const SourcePosition& position() const { return position_; }
    /** Returns the type of this token. */
    const TokenType& type() const { return type_; }
    /** Represents the value of this token. */
    const std::u32string& value() const { return value_; }

    /** Returns a string describing the token */
    const char* stringName() const;
    /** Returns a string describing the token */
    static const char* stringNameForType(TokenType type);

    void validate() const;
private:
    SourcePosition position_;
    TokenType type_ = TokenType::NoType;
    std::u32string value_;
};

}  // namespace EmojicodeCompiler

#endif /* Token_hpp */
