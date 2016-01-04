//
//  CompilerScope.h
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef __Emojicode__CompilerScope__
#define __Emojicode__CompilerScope__

#include "EmojicodeCompiler.h"

ScopeWrapper *currentScopeWrapper;

/** A variable scope */
struct Scope {
    /** 
     * This map contains the variables and values.
     * @warning Do not change directly. Use @c setVariable, and @c getVariable.
     * @private
     */
    std::map<EmojicodeString, CompilerVariable*> map;
    /** Whether the top scopes shall be accessable. */
    bool stop;
};

struct ScopeWrapper {
    struct ScopeWrapper *topScope;
    Scope *scope;
};


/** Returns the base scope. */
void initScope();

/** Create a new subscope */
Scope* newSubscope(bool stop);

/** Pops current scope and returns it */
Scope* popScope();

/** Push this scope to be the current scope. */
void pushScope(Scope *scope);

/** Releases the scope and does garabge collecting. */
void releaseScope(Scope *s);

/**
 * Retrieves a variable.
 */
CompilerVariable* getVariable(Token *variable, uint8_t *scopesUp);

/**
 * Retrieves a variable form the local scope or @c NULL.
 */
CompilerVariable* getLocalVariable(Token *variable, Scope *scope);

/**
 * Sets a variable.
 * All scopes will be searched, if the variable was set in a top scope before it will receive the new value. If the variable was not set before it will be set in the current scope.
 */
void setVariable(Token *variable, CompilerVariable *value);

/**
 * Sets a variable directly to the passed scope. Make sure you really intend to use this function!
 */
void setLocalVariable(Token *variable, CompilerVariable *value, Scope *scope);

#endif /* defined(__Emojicode__Scope__) */
