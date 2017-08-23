//
//  Lexer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Lexer_hpp
#define Lexer_hpp

#include "TokenStream.hpp"
#include <string>

namespace EmojicodeCompiler {

class Lexer {
public:
    Lexer(std::string str, std::string sourcePositionFile) : string_(std::move(str)) {
        sourcePosition_.file = std::move(sourcePositionFile);
    }
    static TokenStream lexFile(const std::string &path);
    /// Lexes the string and returns a TokenStream
    TokenStream lex();
private:
    enum class TokenState {
        Continues, Ended, NextBegun, Discard
    };

    SourcePosition sourcePosition_ = SourcePosition(1, 0, "");
    /// Checks for a whitespace character and updates ::sourcePosition_.
    /// @returns True if @c is whitespace according to isWhitespace().
    bool detectWhitespace();

    /// Called if a new code point is available.
    /// @returns True if the token continues (e.g. a string) or false if the token ended (i.e. it only consits of this
    /// single code point).
    bool beginToken(Token *token);
    /// Called if a new code point is available and beginToken returned true and all previous calls for this token
    /// returned TokenState::Continues.
    TokenState continueToken(Token *token);
    /// Reads exactly one token.
    /// This method calls nextChar() as necessary. On return, ::codePoint_ already contains the next code point for
    /// another call to beginToken(), if ::continue_ is true.
    void readToken(Token *token);

    bool isNewline() { return codePoint_ == 0x0A || codePoint_ == 0x2028 || codePoint_ == 0x2029; }

    void nextChar();
    bool hasMoreChars() { return i_ < string_.size(); }
    void nextCharOrEnd();

    void singleToken(Token *token, TokenType type, EmojicodeChar c);

    bool isHex_ = false;
    bool escapeSequence_ = false;
    bool foundZWJ_ = false;

    bool continue_ = true;
    EmojicodeChar codePoint_ = 0;

    std::string string_;
    size_t i_ = 0;
    std::vector<Token> tokens_;
};

}  // namespace EmojicodeCompiler

#endif /* Lexer_hpp */
