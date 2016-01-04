//
//  CompilerScope.c
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "CompilerScope.h"

Scope* newSubscope(bool stop){
    Scope *ss = malloc(sizeof(Scope));
    ss->map = newDictionary();
    ss->stop = stop;
    return ss;
}

Scope* popScope(){
    ScopeWrapper *sw = currentScopeWrapper;
    currentScopeWrapper = currentScopeWrapper->topScope;
    Scope *s = sw->scope;
    free(sw);
    return s;
}

void releaseScope(Scope *s){
    dictionaryFree(s->map, free);
    free(s);
}

void pushScope(Scope *scope){
    ScopeWrapper *sw = malloc(sizeof(ScopeWrapper));
    sw->topScope = currentScopeWrapper;
    sw->scope = scope;
    currentScopeWrapper = sw;
}

void setVariable(Token *variable, CompilerVariable *value){
    //Search all scopes up
    for (ScopeWrapper *scopeWrapper = currentScopeWrapper; scopeWrapper != NULL; scopeWrapper = scopeWrapper->topScope) {
        if(dictionaryLookup(scopeWrapper->scope->map, variable->value, variable->valueLength * sizeof(EmojicodeChar)) != NULL){
            dictionarySet(scopeWrapper->scope->map, variable->value, variable->valueLength  * sizeof(EmojicodeChar), value);
            return;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }

    //Current scope never must be NULL - otherwise a crash is legal
    dictionarySet(currentScopeWrapper->scope->map, variable->value, variable->valueLength * sizeof(EmojicodeChar), value);
}

void setLocalVariable(Token *variable, CompilerVariable *value, Scope *scope){
    dictionarySet(scope->map, variable->value, variable->valueLength * sizeof(EmojicodeChar), value);
}

CompilerVariable* getVariable(Token *variable, uint8_t *scopesUp){
    *scopesUp = 0;
    
    CompilerVariable *value;
    ScopeWrapper *scopeWrapper = currentScopeWrapper;
    for (; scopeWrapper != NULL; scopeWrapper = scopeWrapper->topScope, (*scopesUp)++) {
        if((value = dictionaryLookup(scopeWrapper->scope->map, variable->value, variable->valueLength * sizeof(EmojicodeChar))) != NULL){
            return value;
        }
        if (scopeWrapper->scope->stop) {
            break;
        }
    }
    
    return NULL;
}

CompilerVariable* getLocalVariable(Token *variable, Scope *scope){
    return dictionaryLookup(scope->map, variable->value, variable->valueLength * sizeof(EmojicodeChar));
}

