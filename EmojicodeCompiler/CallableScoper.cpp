//
//  CallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include "CallableScoper.hpp"
#include "VariableNotFoundErrorException.hpp"

Scope& CallableScoper::currentScope() {
    return scopes_.front();
}

void CallableScoper::popScopeAndRecommendFrozenVariables() {
    auto &scope = currentScope();
    scope.recommendFrozenVariables();
    reduceOffsetBy(static_cast<int>(scope.size()));
    scopes_.pop_front();
}

Scope& CallableScoper::pushScope() {
    scopes_.push_front(Scope(this));
    return scopes_.front();
}

Scope& CallableScoper::topmostLocalScope() {
    return scopes_.back();
}

std::pair<Variable&, bool> CallableScoper::getVariable(const EmojicodeString &name, SourcePosition errorPosition) {
    for (Scope &scope : scopes_) {
        if (scope.hasLocalVariable(name)) {
            return std::pair<Variable&, bool>(scope.getLocalVariable(name), false);
        }
    }
    if (instanceScope_ && instanceScope_->hasLocalVariable(name)) {
        return std::pair<Variable&, bool>(instanceScope_->getLocalVariable(name), true);
    }
    throw VariableNotFoundErrorException(errorPosition, name);
}
