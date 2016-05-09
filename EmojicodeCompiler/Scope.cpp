//
//  Scope.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scope.hpp"
#include "CompilerErrorException.hpp"

void Scope::changeInitializedBy(int c) {
    for (auto &it : map_) {
        Variable &cv = it.second;
        if (cv.initialized > 0) {
            cv.initialized += c;
        }
    }
}

void Scope::setLocalVariable(const EmojicodeString &variable, Variable value) {
    map_.emplace(variable, value);
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
            const char *variableName = cv.definitionToken.value.utf8CString();
            throw CompilerErrorException(errorToken, errorMessage, variableName);
        }
    }
}

void Scope::recommendFrozenVariables() const {
    for (auto &it : map_) {
        const Variable &cv = it.second;
        if (!cv.frozen() && !cv.mutated()) {
            const char *variableName = cv.definitionToken.value.utf8CString();
            compilerWarning(cv.definitionToken,
                            "Variable \"%s\" was never mutated; consider making it a frozen 🍦 variable.",
                            variableName);
        }
    }
}
