//
//  CompilerScope.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "CompilerScope.hpp"
#include "Lexer.hpp"


//MARK: Compiler Variables

void CompilerVariable::uninitalizedError(const Token *variableToken) const {
    if (initialized <= 0) {
        compilerError(variableToken, "Variable \"%s\" is possibly not initialized.", variableToken->value.utf8CString());
    }
}

void CompilerVariable::frozenError(const Token *variableToken) const {
    if (frozen) {
        compilerError(variableToken, "Cannot modify frozen variable \"%s\".", variableToken->value.utf8CString());
    }
}

ScopeWrapper *currentScopeWrapper;

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

void Scope::setLocalVariable(const Token *variable, CompilerVariable *value){
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(variable->value, value));
}

void Scope::setLocalVariable(EmojicodeString string, CompilerVariable *value){
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(string, value));
}

CompilerVariable* Scope::getLocalVariable(const Token *variable){
    auto it = map.find(variable->value);
    return it == map.end() ? nullptr : it->second;
}

/** Emits @c errorMessage if not all instance variable were initialized. @c errorMessage should include @c %s for the name of the variable. */
void Scope::initializerUnintializedVariablesCheck(const Token *errorToken, const char *errorMessage){
    for (auto it : map) {
        CompilerVariable *cv = it.second;
        if (cv->initialized <= 0 && !cv->type.optional) {
            const char *variableName = cv->variable->name->value.utf8CString();
            compilerError(errorToken, errorMessage, variableName);
        }
    }
}

int Scope::copyFromScope(Scope *copyScope, uint8_t offsetID){
    for (auto it : copyScope->map) {
        auto *var = new CompilerVariable(it.second->type, offsetID + it.second->id, it.second->initialized, true);
        setLocalVariable(it.first, var);
    }
    return (int)copyScope->map.size();
}

Scope* popScope(){
    ScopeWrapper *sw = currentScopeWrapper;
    currentScopeWrapper = currentScopeWrapper->topScope;
    Scope *s = sw->scope;
    delete sw;
    return s;
}

void pushScope(Scope *scope){
    ScopeWrapper *sw = new ScopeWrapper;
    sw->topScope = currentScopeWrapper;
    sw->scope = scope;
    currentScopeWrapper = sw;
}

void setVariable(const Token *variable, CompilerVariable *value){
    //Search all scopes up
    for (ScopeWrapper *scopeWrapper = currentScopeWrapper; scopeWrapper != nullptr; scopeWrapper = scopeWrapper->topScope) {
        if(scopeWrapper->scope->getLocalVariable(variable) != nullptr){
            scopeWrapper->scope->setLocalVariable(variable, value);
            return;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }

    currentScopeWrapper->scope->setLocalVariable(variable, value);
}

CompilerVariable* getVariable(const Token *variable, uint8_t *scopesUp){
    *scopesUp = 0;
    
    CompilerVariable *value;
    ScopeWrapper *scopeWrapper = currentScopeWrapper;
    for (; scopeWrapper != nullptr; scopeWrapper = scopeWrapper->topScope, (*scopesUp)++) {
        if((value = scopeWrapper->scope->getLocalVariable(variable)) != nullptr){
            return value;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }
    
    return nullptr;
}



