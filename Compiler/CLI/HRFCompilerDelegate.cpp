//
//  HRFCompilerDelegate.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "HRFCompilerDelegate.hpp"
#include "Lex/SourcePosition.hpp"
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

void HRFCompilerDelegate::error(const SourcePosition &p, const std::string &message) {
    std::cerr << "🚨 line " << p.line << " column " << p.character << " " << p.file << ": " << message << std::endl;
}

void HRFCompilerDelegate::warn(const SourcePosition &p, const std::string &message) {
    std::cerr << "⚠️ line " << p.line << " column " << p.character << " " << p.file << ": " << message << std::endl;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
