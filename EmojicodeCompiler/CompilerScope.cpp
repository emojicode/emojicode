//
//  CompilerScope.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "CompilerScope.h"

Scope* newSubscope(bool stop){
    Scope *s = new Scope;
    s->stop = stop;
    return s;
}

Scope* popScope(){
    ScopeWrapper *sw = currentScopeWrapper;
    currentScopeWrapper = currentScopeWrapper->topScope;
    Scope *s = sw->scope;
    free(sw);
    return s;
}

void releaseScope(Scope *s){
    delete s;
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
        if(scopeWrapper->scope->map.count(variable->value)){
            setLocalVariable(variable, value, scopeWrapper->scope);
            return;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }

    setLocalVariable(variable, value, currentScopeWrapper->scope);
}

void setLocalVariable(Token *variable, CompilerVariable *value, Scope *scope){
    scope->map.insert(std::map<EmojicodeString, CompilerVariable*>::value_type(variable->value, value));
}

CompilerVariable* getVariable(Token *variable, uint8_t *scopesUp){
    *scopesUp = 0;
    
    CompilerVariable *value;
    ScopeWrapper *scopeWrapper = currentScopeWrapper;
    for (; scopeWrapper != NULL; scopeWrapper = scopeWrapper->topScope, (*scopesUp)++) {
        if((value = getLocalVariable(variable, scopeWrapper->scope)) != NULL){
            return value;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }
    
    return NULL;
}

CompilerVariable* getLocalVariable(Token *variable, Scope *scope){
    auto i = scope->map.find(variable->value);
    return i == scope->map.end() ? NULL : i->second;
}

