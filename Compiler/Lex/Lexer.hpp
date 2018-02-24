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
/// Lexer also returns tokens representing comments.
class Lexer {
public:
    /// @param sourceCode The Emojicode source code that shall be analyzed.
    /// @param sourcePositionFile The name or path to the file that contained the source code. This string is used as
    ///                           SourcePosition::file value for the position of all tokens created by this Lexer.
    Lexer(std::u32string sourceCode, std::string sourcePositionFile);

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

    void loadOperatorSingleTokens();

    SourcePosition sourcePosition_;

    /// Checks for a whitespace character and updates ::sourcePosition_.
    /// @returns True if @c is whitespace according to isWhitespace().
    /// @warning This method must not be called more than once for the same character!
    bool detectWhitespace();

    /// Called if a new code point is available.
    /// @returns True if the token continues (e.g. a string) or false if the token ended (i.e. it only consists of this
    /// single code point).
    bool beginToken(Token *token);

    /// Called if a new code point is available and beginToken returned true and all previous calls for this token
    /// returned TokenState::Continues.
    TokenState continueToken(Token *token);

    /// Reads exactly one token.
    /// This method calls nextChar() as necessary. On return, ::codePoint() already returns the next code point for
    /// another call to beginToken(), if ::continue_ is true.
    void readToken(Token *token);

    bool isNewline() { return codePoint() == 0x0A || codePoint() == 0x2028 || codePoint() == 0x2029; }

    /// @returns True if the code point is  a whitespace character.
    /// See http://www.unicode.org/Public/6.3.0/ucd/PropList.txt
    bool isWhitespace() {
        return (0x9 <= codePoint() && codePoint() <= 0xD) || codePoint() == 0x20 || codePoint() == 0x85
               || codePoint() == 0xA0 || codePoint() == 0x1680 || (0x2000 <= codePoint() && codePoint() <= 0x200A)
               || codePoint() == 0x2028 || codePoint() == 0x2029 || codePoint() == 0x2029 || codePoint() == 0x202F
               || codePoint() == 0x205F || codePoint() == 0x3000 || codePoint() == 0xFE0F;
    }

    /// Loads the first character and removes leading whitespace.
    void prepare();

    void nextChar();

    bool hasMoreChars() { return i_ < string_.size(); }

    void nextCharOrEnd();

    char32_t codePoint() { return codePoint_; }

    bool isHex_ = false;
    bool escapeSequence_ = false;
    bool foundZWJ_ = false;

    bool continue_ = true;

    char32_t codePoint_ = 0;
    std::u32string string_;
    size_t i_ = 0;

    std::map<char32_t, TokenType> singleTokens_;
};

}  // namespace EmojicodeCompiler

#endif /* Lexer_hpp */
