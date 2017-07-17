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

namespace EmojicodeCompiler {

void CallableScoper::popScope(InstructionCount count) {
    auto &scope = currentScope();
    scope.recommendFrozenVariables();
    reduceOffsetBy(static_cast<int>(scope.size()));

    for (Scope &scope : scopes_) {
        for (auto variable : scope.map()) {
            if (variable.second.initializationLevel() == 1) {
                variable.second.type().objectVariableRecords(variable.second.id(), *info_,
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
    if (pushedTemporaryScope_) {
        throw std::logic_error("Pushing on temporary scope");
    }
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

}  // namespace EmojicodeCompiler
