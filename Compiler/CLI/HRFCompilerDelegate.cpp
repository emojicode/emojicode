//
//  HRFCompilerDelegate.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "HRFCompilerDelegate.hpp"
#include "Utils/rang.hpp"
#include "Lex/SourcePosition.hpp"

namespace EmojicodeCompiler {

namespace CLI {

HRFCompilerDelegate::HRFCompilerDelegate(bool forceColor) {
    if (forceColor) {
        rang::setControlMode(rang::control::Force);
    }
}

void HRFCompilerDelegate::error(Compiler *compiler, const std::string &message, const SourcePosition &p) {
    printPosition(p);
    std::cerr << rang::fg::red << "🚨 error: " << rang::style::reset;
    printMessage(message);
    printOffendingCode(compiler, p);
}

void HRFCompilerDelegate::printMessage(const std::string &message) const {
    std::cerr << rang::style::bold << message << std::endl << rang::style::reset;
}

void HRFCompilerDelegate::printPosition(const SourcePosition &p) const {
    std::cerr << rang::style::bold;
    if (p.file != nullptr) {
        std::cerr << p.file->path() << ":" << p.line << ":" << p.character << ": ";
    }
}

void HRFCompilerDelegate::warn(Compiler *compiler, const std::string &message, const SourcePosition &p) {
    printPosition(p);
    std::cerr << rang::fg::yellow << "⚠️  warning: " << rang::style::reset;
    printMessage(message);
    printOffendingCode(compiler, p);
}

void HRFCompilerDelegate::printOffendingCode(Compiler *compiler, const SourcePosition &position) {
    auto line = position.wholeLine();
    if (line.empty()) {
        return;
    }
    std::cerr << utf8(line);
    auto r = position.character > 0 ? position.character - 1 : 0;
    std::cerr << std::string(r, ' ') << "⬆️" << std::endl << std::endl;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
