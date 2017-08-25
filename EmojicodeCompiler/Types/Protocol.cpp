//
//  Protocol.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Protocol.hpp"

#include "../Application.hpp"
#include "../Functions/Function.hpp"
#include <utility>

namespace EmojicodeCompiler {

Protocol::Protocol(std::u32string name, Package *pkg, const SourcePosition &p, const std::u32string &string)
    : TypeDefinition(std::move(name), pkg, p, string) {
    index = package()->app()->protocolIndex();
}

void Protocol::addMethod(Function *method) {
    method->setVti(static_cast<int>(methodList().size()));
    TypeDefinition::addMethod(method);
}

}  // namespace EmojicodeCompiler
