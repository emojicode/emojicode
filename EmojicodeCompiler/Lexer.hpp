//
//  Lexer.h
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Lexer_hpp
#define Lexer_hpp

#include "EmojicodeCompiler.hpp"
#include "Token.hpp"
#include "TokenStream.hpp"

TokenStream lex(const char* fileName);

#endif /* Lexer_hpp */
