//
//  CallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "VariableNotFoundError.hpp"
#include "CallableScoper.hpp"
#include "Function.hpp"
#include <map>

Scope& CallableScoper::currentScope() {
    return scopes_.front();
}

void CallableScoper::popScopeAndRecommendFrozenVariables(std::vector<FunctionObjectVariableInformation> &info,
                                                         InstructionCount count) {
    auto &scope = currentScope();
    scope.recommendFrozenVariables();
    reduceOffsetBy(static_cast<int>(scope.size()));

    for (Scope &scope : scopes_) {
        for (auto variable : scope.map()) {
            if (variable.second.initializationLevel() == 1) {
                variable.second.type().objectVariableRecords(variable.second.id(), info,
                                                             variable.second.initializationPosition(), count);
            }
        }
    }
    scopes_.pop_front();

    maxInitializationLevel_--;
    for (auto &scope : scopes_) {
        scope.popInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->popInitializationLevel();
    }
}

Scope& CallableScoper::pushScope() {
    maxInitializationLevel_++;
    for (auto &scope : scopes_) {
        scope.pushInitializationLevel();
    }
    if (instanceScope() != nullptr) {
        instanceScope()->pushInitializationLevel();
    }

    scopes_.push_front(Scope(this));
    return scopes_.front();
}

Scope& CallableScoper::topmostLocalScope() {
    return scopes_.back();
}

ResolvedVariable CallableScoper::getVariable(const EmojicodeString &name, const SourcePosition &errorPosition) {
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
