//
//  CompilerScope.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "CompilerScope.h"
#include "Lexer.h"

ScopeWrapper *currentScopeWrapper;

void Scope::changeInitializedBy(int c) {
    for (auto it : map) {
        CompilerVariable *cv = it.second;
        if (cv->initialized > 0) {
            cv->initialized += c;
        }
    }
}

void Scope::setLocalVariable(Token *variable, CompilerVariable *value){
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(variable->value, value));
}

void Scope::setLocalVariable(EmojicodeString string, CompilerVariable *value){
    map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(string, value));
}

CompilerVariable* Scope::getLocalVariable(Token *variable){
    auto it = map.find(variable->value);
    return it == map.end() ? NULL : it->second;
}

/** Emits @c errorMessage if not all instance variable were initialized. @c errorMessage should include @c %s for the name of the variable. */
void Scope::initializerUnintializedVariablesCheck(Token *errorToken, const char *errorMessage){
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
    free(sw);
    return s;
}

void pushScope(Scope *scope){
    ScopeWrapper *sw = new ScopeWrapper;
    sw->topScope = currentScopeWrapper;
    sw->scope = scope;
    currentScopeWrapper = sw;
}

void setVariable(Token *variable, CompilerVariable *value){
    //Search all scopes up
    for (ScopeWrapper *scopeWrapper = currentScopeWrapper; scopeWrapper != NULL; scopeWrapper = scopeWrapper->topScope) {
        if(scopeWrapper->scope->getLocalVariable(variable) != NULL){
            scopeWrapper->scope->setLocalVariable(variable, value);
            return;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }

    currentScopeWrapper->scope->setLocalVariable(variable, value);
}

CompilerVariable* getVariable(Token *variable, uint8_t *scopesUp){
    *scopesUp = 0;
    
    CompilerVariable *value;
    ScopeWrapper *scopeWrapper = currentScopeWrapper;
    for (; scopeWrapper != NULL; scopeWrapper = scopeWrapper->topScope, (*scopesUp)++) {
        if((value = scopeWrapper->scope->getLocalVariable(variable)) != NULL){
            return value;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }
    
    return NULL;
}



