//
//  Protocol.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Protocol.hpp"
#include "../Functions/Function.hpp"
#include <utility>

namespace EmojicodeCompiler {

Protocol::Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string,
                   bool exported)
    : TypeDefinition(std::move(name), pkg, p, string, exported) {}

void Protocol::prepareForSemanticAnalysis() {
    for (size_t i = 0; i < methodList().size(); i++) {
        methodList()[i]->setVti(static_cast<int>(i));
    }
}

}  // namespace EmojicodeCompiler
