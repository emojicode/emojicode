//
//  Scope.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scope.hpp"
#include "CompilerErrorException.hpp"
#include "TypeDefinition.hpp"
#include "Scoper.hpp"

void Scope::setVariableInitialization(bool initd) {
    for (auto &it : map_) {
        it.second.initialized = initd ? 1 : 0;
    }
}

void Scope::pushInitializationLevel() {
    for (auto &it : map_) {
        Variable &cv = it.second;
        if (cv.initialized > 0) {
            cv.initialized++;
        }
    }
}

void Scope::popInitializationLevel() {
    for (auto &it : map_) {
        Variable &cv = it.second;
        if (cv.initialized > 0) {
            cv.initialized--;
        }
    }
}

Variable& Scope::setLocalVariable(const EmojicodeString &variable, Type type, bool frozen, SourcePosition pos,
                                  bool initialized) {
    if (hasLocalVariable(variable)) {
        return getLocalVariable(variable);
    }
    int id = scoper_->reserveVariable(type.size());
    Variable &v = map_.emplace(variable, Variable(type, id, initialized ? 1 : 0, frozen, variable, pos)).first->second;
    size_ += type.size();
    return v;
}

Variable& Scope::getLocalVariable(const EmojicodeString &variable) {
    return map_.find(variable)->second;
}

bool Scope::hasLocalVariable(const EmojicodeString &variable) const {
    return map_.count(variable) > 0;
}

void Scope::initializerUnintializedVariablesCheck(const Token &errorToken, const char *errorMessage) {
    for (auto &it : map_) {
        Variable &cv = it.second;
        if (cv.initialized <= 0 && !cv.type.optional()) {
            throw CompilerErrorException(errorToken, errorMessage, cv.string_.utf8().c_str());
        }
    }
}

void Scope::recommendFrozenVariables() const {
    for (auto &it : map_) {
        const Variable &cv = it.second;
        if (!cv.frozen() && !cv.mutated()) {
            compilerWarning(cv.position(),
                            "Variable \"%s\" was never mutated; consider making it a frozen 🍦 variable.",
                            cv.string_.utf8().c_str());
        }
    }
}
