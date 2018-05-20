//
//  JSONCompilerDelegate.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "JSONCompilerDelegate.hpp"
#include "Lex/SourcePosition.hpp"
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

JSONCompilerDelegate::JSONCompilerDelegate() : wrapper_(std::cout), writer_(wrapper_) {}

void JSONCompilerDelegate::begin() {
    writer_.StartArray();
}

void JSONCompilerDelegate::finish() {
    writer_.EndArray();
}

void JSONCompilerDelegate::printJson(const char *type, const SourcePosition &p, const std::string &message) {
    writer_.StartObject();
    writer_.Key("type");
    writer_.String(type);
    writer_.Key("line");
    writer_.Uint64(p.line);
    writer_.Key("character");
    writer_.Uint64(p.character);
    writer_.Key("file");
    writer_.String(p.file == nullptr ? "" : p.file->path());
    writer_.Key("message");
    writer_.String(message);
    writer_.EndObject();
}

void JSONCompilerDelegate::error(Compiler *compiler, const std::string &message, const SourcePosition &p) {
    printJson("error", p, message);
}

void JSONCompilerDelegate::warn(Compiler *compiler, const std::string &message, const SourcePosition &p) {
    printJson("warning", p, message);
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
