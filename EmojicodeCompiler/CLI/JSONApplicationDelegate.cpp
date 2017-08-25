//
//  JSONApplicationDelegate.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "JSONApplicationDelegate.hpp"
#include "../Lex/SourcePosition.hpp"
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

void JSONApplicationDelegate::begin() {
    std::cerr << "[" << std::endl;
}

void JSONApplicationDelegate::finish() {
    std::cerr << "]" << std::endl;
}

void JSONApplicationDelegate::printJson(const char *type, const SourcePosition &p, const std::string &message) {
    printer_.print();
    std::cerr << "{\"type\": \"" << type << "\", \"line\": " << p.line << ", \"character\": " << p.character;
    std::cerr << ", \"file\":";
    jsonString(p.file, std::cerr);
    std::cerr << ", \"message\": ";
    jsonString(message, std::cerr);
    std::cerr << "}" << std::endl;
}

void JSONApplicationDelegate::error(const SourcePosition &p, const std::string &message) {
    printJson("error", p, message);
}

void JSONApplicationDelegate::warn(const SourcePosition &p, const std::string &message) {
    printJson("warning", p, message);
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
