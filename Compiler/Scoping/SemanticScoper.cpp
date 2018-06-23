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
#include <map>

namespace EmojicodeCompiler {

Scope& SemanticScoper::pushArgumentsScope(const std::vector<Parameter> &arguments, const SourcePosition &p) {
    Scope &methodScope = pushScope();
    for (auto &variable : arguments) {
        auto &var = methodScope.declareVariable(variable.name, variable.type->type(), true, p);
        var.initializeAbsolutely();
    }
    return methodScope;
}

void SemanticScoper::popScope(Compiler *app) {
    currentScope().recommendFrozenVariables(app);

    updateMaxVariableIdForPopping();
    scopes_.pop_front();

    maxInitializationLevel_--;
    for (auto &scope : scopes_) {
        scope.popInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->popInitializationLevel();
    }
}

SemanticScoper SemanticScoper::scoperForFunction(Function *function)  {
    if (hasInstanceScope(function->functionType())) {
        return SemanticScoper(&function->owner()->instanceScope());
    }
    return SemanticScoper();
}

Scope& SemanticScoper::pushScope() {
    maxInitializationLevel_++;
    for (auto &scope : scopes_) {
        scope.pushInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->pushInitializationLevel();
    }
    pushScopeInternal();
    return scopes_.front();
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

}  // namespace EmojicodeCompiler
