//
//  CallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "VariableNotFoundError.hpp"
#include "Functions/Function.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/TypeDefinition.hpp"
#include "Compiler.hpp"
#include <map>

namespace EmojicodeCompiler {

Scope& SemanticScoper::pushArgumentsScope(const std::vector<Parameter> &arguments, const SourcePosition &p) {
    pushScope();
    for (auto &variable : arguments) {
        auto &var = currentScope().declareVariable(variable.name, variable.type->type(), true, p);
        var.initializeAbsolutely();
    }
    return currentScope();
}

SemanticScopeStats SemanticScoper::popScope(Compiler *compiler) {
    currentScope().recommendFrozenVariables(compiler);

    auto count = scopes_.front().map().size();
    auto maxVariableId = scopes_.front().maxVariableId();

    updateMaxVariableIdForPopping();
    scopes_.pop_front();

    maxInitializationLevel_--;
    for (auto &scope : scopes_) {
        scope.popInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->popInitializationLevel();
    }
    return { scopes_.empty() ? 0 : scopes_.front().maxVariableId(), count, maxVariableId };
}

SemanticScoper SemanticScoper::scoperForFunction(Function *function)  {
    if (hasInstanceScope(function->functionType())) {
        return SemanticScoper(&function->owner()->instanceScope());
    }
    return SemanticScoper();
}

void SemanticScoper::pushScope() {
    maxInitializationLevel_++;
    for (auto &scope : scopes_) {
        scope.pushInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->pushInitializationLevel();
    }
    pushScopeInternal();
}

ResolvedVariable SemanticScoper::getVariable(const std::u32string &name, const SourcePosition &errorPosition) {
    for (Scope &scope : scopes_) {
        if (scope.hasLocalVariable(name)) {
            return ResolvedVariable(scope.getLocalVariable(name), false);
        }
    }
    if (instanceScope_ != nullptr && instanceScope_->hasLocalVariable(name)) {
        return ResolvedVariable(instanceScope_->getLocalVariable(name), true);
    }
    throw VariableNotFoundError(errorPosition, name);
}

void SemanticScoper::checkForShadowing(const std::u32string &name, const SourcePosition &p, Compiler *compiler) const {
    for (const Scope &scope : scopes_) {
        if (scope.hasLocalVariable(name)) {
            compiler->warn(p, "Declaration of ", utf8(name), " shadows previous local variable.");
        }
    }
    if (instanceScope_ != nullptr && instanceScope_->hasLocalVariable(name)) {
        compiler->warn(p, "Declaration of ", utf8(name), " shadows instance variable.");
    }
}

}  // namespace EmojicodeCompiler
