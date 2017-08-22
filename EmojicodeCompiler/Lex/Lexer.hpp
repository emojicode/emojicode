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
    bool detectWhitespace(EmojicodeChar c);
    /// Called if a new character is available and beginToken returned true and all previous calls for this token
    /// returned TokenState::Continues.
    TokenState continueToken(EmojicodeChar c, Token *token);

    bool beginToken(EmojicodeChar c, Token *token);

    void nextChar();
    bool hasMoreChars() { return i_ < string_.size(); }
    void nextCharOrEnd();

    void singleToken(Token *token, TokenType type, EmojicodeChar c);

    bool isHex_ = false;
    bool escapeSequence_ = false;
    bool foundZWJ_ = false;

    bool continue_ = true;
    EmojicodeChar c;

    std::string string_;
    size_t i_ = 0;
    std::vector<Token> tokens_;
};

}  // namespace EmojicodeCompiler

#endif /* Lexer_hpp */
