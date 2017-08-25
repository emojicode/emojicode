//
//  HRFApplicationDelegate.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "HRFApplicationDelegate.hpp"
#include "../Lex/SourcePosition.hpp"
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

void HRFApplicationDelegate::error(const SourcePosition &p, const std::string &message) {
    std::cerr << "🚨 line " << p.line << " column " << p.character << " " << p.file << ": " << message << std::endl;
}

void HRFApplicationDelegate::warn(const SourcePosition &p, const std::string &message) {
    std::cerr << "⚠️ line " << p.line << " column " << p.character << " " << p.file << ": " << message << std::endl;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
