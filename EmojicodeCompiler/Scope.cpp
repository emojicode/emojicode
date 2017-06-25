//
//  Scope.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scope.hpp"
#include "CompilerError.hpp"
#include "Scoper.hpp"
#include "TypeDefinition.hpp"

void Scope::setVariableInitialization(bool initd) {
    for (auto &it : map_) {
        if (initd) {
            it.second.initialize(0);
        }
        else {
            it.second.uninitialize();
        }
    }
}

void Scope::pushInitializationLevel() {
    for (auto &it : map_) {
        it.second.pushInitializationLevel();
    }
}

void Scope::popInitializationLevel() {
    for (auto &it : map_) {
        it.second.popInitializationLevel();
    }
}

Variable& Scope::setLocalVariable(const EmojicodeString &variable, const Type &type, bool frozen,
                                  const SourcePosition &p) {
    if (hasLocalVariable(variable)) {
        throw CompilerError(p, "Cannot redeclare variable.");
    }
    int id = scoper_->reserveVariable(type.size());
    Variable &v = map_.emplace(variable, Variable(type, id, frozen, variable, p)).first->second;
    size_ += type.size();
    return v;
}

Variable& Scope::setLocalVariableWithID(const EmojicodeString &variable, const Type &type, bool frozen, int id,
                                        const SourcePosition &p) {
    if (hasLocalVariable(variable)) {
        throw CompilerError(p, "Cannot redeclare variable.");
    }
    Variable &v = map_.emplace(variable, Variable(type, id, frozen, variable, p)).first->second;
    return v;
}

int Scope::allocateInternalVariable(const Type &type) {
    int id = scoper_->reserveVariable(type.size());
    size_ += type.size();
    return id;
}

Variable& Scope::getLocalVariable(const EmojicodeString &variable) {
    return map_.find(variable)->second;
}

bool Scope::hasLocalVariable(const EmojicodeString &variable) const {
    return map_.count(variable) > 0;
}

void Scope::initializerUnintializedVariablesCheck(const SourcePosition &p, const char *errorMessage) {
    for (auto &it : map_) {
        Variable &cv = it.second;
        if (!cv.initialized() && !cv.type().optional() && !cv.inherited()) {
            throw CompilerError(p, errorMessage, cv.name().utf8().c_str());
        }
    }
}

void Scope::recommendFrozenVariables() const {
    for (auto &it : map_) {
        const Variable &cv = it.second;
        if (!cv.frozen() && !cv.mutated()) {
            compilerWarning(cv.position(),
                            "Variable \"%s\" was never mutated; consider making it a frozen 🍦 variable.",
                            cv.name().utf8().c_str());
        }
    }
}
