//
//  TypeDefinition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinition.hpp"
#include "CompilerError.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Type.hpp"
#include "Scoping/Scope.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

TypeDefinition::TypeDefinition(std::u32string name, Package *p, SourcePosition pos, std::u32string documentation,
                               bool exported)
    : Definition(p, std::move(documentation), std::move(name), pos),
      scope_(std::make_unique<Scope>()), exported_(exported) {}

TypeDefinition::~TypeDefinition() = default;

void TypeDefinition::addInstanceVariable(const InstanceVariableDeclaration &variable) {
    auto duplicate = std::find_if(instanceVariables_.begin(), instanceVariables_.end(),
                                  [&variable](auto &b) { return variable.name == b.name; });
    if (duplicate != instanceVariables_.end()) {
        throw CompilerError(variable.position, "Redeclaration of instance variable.");
    }
    instanceVariables_.push_back(variable);
}

void TypeDefinition::eachFunction(const std::function<void (Function *)>& cb) const {
    eachFunctionWithoutInitializers(cb);
    for (auto function : inits().list()) {
        cb(function);
    }
}

void TypeDefinition::eachFunctionWithoutInitializers(const std::function<void (Function *)>& cb) const {
    for (auto function : methods().list()) {
        cb(function);
    }
    for (auto function : typeMethods().list()) {
        cb(function);
    }
}

}  // namespace EmojicodeCompiler
