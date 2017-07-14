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

TokenStream lex(const std::string &path);

}  // namespace EmojicodeCompiler

#endif /* Lexer_hpp */
