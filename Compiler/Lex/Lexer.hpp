//
//  Lexer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Lexer_hpp
#define Lexer_hpp

#include "Token.hpp"
#include "SourceManager.hpp"
#include <map>
#include <string>

namespace EmojicodeCompiler {

/// Lexer is responsible for the lexical analysis of a source code file.
///
/// It takes the content of the source code file as a string and provides three methods: lex() which returns the next
/// token, continues() to determine if there are more tokens and position() to determine the position of the lexer.
///
/// Lexer is normally not directly accessed but used through TokenStream, which provides a more convenient interface.
///
/// Lexer also returns tokens representing comments and line breaks.
class Lexer {
public:
    /// @param sourceCode The Emojicode source code that shall be analyzed.
    Lexer(SourceFile *source);

    /// @returns The next token.
    /// @throws CompilerError if an error occurs during tokenization.
    /// @pre continues() must be true.
    Token lex();

    /// @returns True iff characters to be tokenized are left.
    bool continues() const { return continue_; }

    /// @returns The position which would be the position of the next token returned by lex().
    const SourcePosition& position() const { return sourcePosition_; }

private:
    enum class TokenState {
        Continues, Ended, NextBegun
    };

    struct TokenConstructionState {
        bool isHex_ = false;
        bool escapeSequence_ = false;
        bool foundZWJ_ = false;
        bool commentDetermined_ = false;
    };

    void loadOperatorSingleTokens();

    /// Checks for a whitespace character and updates ::sourcePosition_.
    /// @returns True if @c is whitespace according to isWhitespace().
    /// @warning This method must not be called more than once for the same character!
    bool detectWhitespace();

    /// Called if a new code point is available.
    /// @returns True if the token continues (e.g. a string) or false if the token ended (i.e. it only consists of this
    /// single code point).
    bool beginToken(Token *token, TokenConstructionState *constState) const;

    /// Called if a new code point is available and beginToken returned true and all previous calls for this token
    /// returned TokenState::Continues.
    TokenState continueToken(Token *token, TokenConstructionState *constState) const;

    /// Reads exactly one token.
    /// This method calls nextChar() as necessary. On return, ::codePoint() already returns the next code point for
    /// another call to beginToken(), if ::continue_ is true.
    Token readToken();

    bool isNewline() const { return codePoint() == 0x0A || codePoint() == 0x2028 || codePoint() == 0x2029; }

    /// @returns True if the code point is  a whitespace character.
    /// See http://www.unicode.org/Public/6.3.0/ucd/PropList.txt
    bool isWhitespace() const {
        return (0x9 <= codePoint() && codePoint() <= 0xD) || codePoint() == 0x20 || codePoint() == 0x85
               || codePoint() == 0xA0 || codePoint() == 0x1680 || (0x2000 <= codePoint() && codePoint() <= 0x200A)
               || codePoint() == 0x2028 || codePoint() == 0x2029 || codePoint() == 0x2029 || codePoint() == 0x202F
               || codePoint() == 0x205F || codePoint() == 0x3000 || codePoint() == 0xFE0F;
    }

    /// Calls nextCharOrEnd() until the codePoint() does not return a whitespace character or the end of the source code
    /// was reached.
    void skipWhitespace();

    /// @returns The current code point to be examined.
    char32_t codePoint() const { return source_->file()[i_]; }

    /// Makes codePoint() provide the next character of the source code.
    /// @throws CompilerError if the end of the source code string was reached, i.e. hasMoreChars() returns false.
    void nextChar();

    /// Makes codePoint() provide the next character of the source code or sets continue_ to false if the end of
    /// the source code was reached, i.e. hasMoreChars() returns false.
    void nextCharOrEnd();

    /// @returns True iff not all characters of the source code string has been tokenized yet.
    bool hasMoreChars() const { return i_ + 1 < source_->file().size(); }

    SourcePosition sourcePosition_;
    bool continue_ = true;
    SourceFile *source_;
    size_t i_ = 0;
    std::map<char32_t, TokenType> singleTokens_;

    TokenState continueIdentifierToken(Token *token, TokenConstructionState *constState) const;

    TokenState continueStringToken(Token *token, TokenConstructionState *constState) const;

    TokenState continueIntegerToken(Token *token, TokenConstructionState *constState) const;

    TokenState continueVariableToken(Token *token) const;

    TokenState continueDoubleToken(Token *token) const;

    TokenState continueOperator(Token *token) const;

    TokenState continueSingleLineToken(Token *token, TokenConstructionState *constState) const;

    TokenState continueMultilineComment(Token *token, TokenConstructionState *constState) const;
    void handleEscapeSequence(Token *token, TokenConstructionState *constState) const;
};

}  // namespace EmojicodeCompiler

#endif /* Lexer_hpp */
