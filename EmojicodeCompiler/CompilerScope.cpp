//
//  CompilerScope.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <string>
#include <map>
#include "CompilerScope.hpp"
#include "Lexer.hpp"

//MARK: Compiler Variables

void CompilerVariable::uninitalizedError(const Token *variableToken) const {
    if (initialized <= 0) {
        compilerError(variableToken,
                      "Variable \"%s\" is possibly not initialized.", variableToken->value.utf8CString());
    }
}

void CompilerVariable::mutate(const Token *variableToken) {
    if (frozen()) {
        compilerError(variableToken, "Cannot modify frozen variable \"%s\".", variableToken->value.utf8CString());
    }
    mutated_ = true;
}

void Scope::changeInitializedBy(int c) {
    for (auto it : map) {
        CompilerVariable *cv = it.second;
        if (cv->initialized > 0) {
            cv->initialized += c;
        }
    }
}

Scope::~Scope() {
    for (auto it : map) {
        delete it.second;
    }
}

void Scope::setLocalVariable(const Token *variable, CompilerVariable *value) {
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(variable->value, value));
}

void Scope::setLocalVariable(EmojicodeString string, CompilerVariable *value) {
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(string, value));
}

CompilerVariable* Scope::getLocalVariable(const Token *variable) {
    auto it = map.find(variable->value);
    return it == map.end() ? nullptr : it->second;
}

void Scope::initializerUnintializedVariablesCheck(const Token *errorToken, const char *errorMessage) {
    for (auto it : map) {
        CompilerVariable *cv = it.second;
        if (cv->initialized <= 0 && !cv->type.optional) {
            const char *variableName = cv->definitionToken->value.utf8CString();
            compilerError(errorToken, errorMessage, variableName);
        }
    }
}

void Scope::recommendFrozenVariables() {
    for (auto it : map) {
        CompilerVariable *cv = it.second;
        if (!cv->frozen() && !cv->mutated()) {
            const char *variableName = cv->definitionToken->value.utf8CString();
            compilerWarning(cv->definitionToken,
                            "Variable \"%s\" was never mutated; consider making it a frozen 🍦 variable.",
                            variableName);
        }
    }
}

int Scope::copyFromScope(Scope *copyScope, uint8_t offsetID) {
    for (auto it : copyScope->map) {
        auto *var = new CompilerVariable(it.second->type, offsetID + it.second->id, it.second->initialized, true, it.second->definitionToken);
        setLocalVariable(it.first, var);
    }
    return (int)copyScope->map.size();
}

void Scoper::setVariable(const Token *variable, CompilerVariable *value) {
    for (auto scope : scopes) {
        if (scope->getLocalVariable(variable)) {
            scope->setLocalVariable(variable, value);
            return;
        }
        if (scope->stop) {
            return;
        }
    }
    
    scopes.front()->setLocalVariable(variable, value);
}

CompilerVariable* Scoper::getVariable(const Token *variable, uint8_t *scopesUp) {
    *scopesUp = 0;
    
    CompilerVariable *cvar;
    for (auto scope : scopes) {
        if ((cvar = scope->getLocalVariable(variable)) != nullptr) {
            return cvar;
        }
        if (scope->stop) {
            break;
        }
        (*scopesUp)++;
    }
    
    return nullptr;
}

Scope* Scoper::currentScope() {
    return scopes.front();
}

Scope* Scoper::topScope() {
    return *std::next(scopes.begin(), 1);
}

void Scoper::popScope() {
    scopes.pop_front();
}

void Scoper::pushScope(Scope *scope) {
    scopes.push_front(scope);
}

