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

extern ScopeWrapper *currentScopeWrapper;

struct CompilerVariable {
public:
    CompilerVariable(Type type, uint8_t id, bool initd, bool frozen) : type(type), initialized(initd), id(id), frozen(frozen) {};
    /** The type of the variable. **/
    Type type;
    /** The ID of the variable. */
    uint8_t id;
    /** The variable is initialized if this field is greater than 0. */
    int initialized;
    /** Set for instance variables. */
    Variable *variable;
    /** Indicating whether variable was frozen. */
    bool frozen;
    
    /** Throws an error if the variable is not initalized. */
    void uninitalizedError(Token *variableToken) const;
    /** Throws an error if the variable is frozen. */
    void frozenError(Token *variableToken) const;
};

/** A variable scope */
struct Scope {
public:
    Scope(bool stop) : stop(stop) {};
    void changeInitializedBy(int c);
    
    /**
     * Sets a variable directly to the scope. Make sure you really intend to use this function!
     */
    void setLocalVariable(Token *variable, CompilerVariable *value);
    void setLocalVariable(EmojicodeString string, CompilerVariable *value);
    
    /**
     * Retrieves a variable form the scope or returns @c NULL.
     */
    CompilerVariable* getLocalVariable(Token *variable);
    
    /** Emits @c errorMessage if not all instance variable were initialized. @c errorMessage should include @c %s for the name of the variable. */
    void initializerUnintializedVariablesCheck(Token *errorToken, const char *errorMessage);
    
    /**
     * Copies the variable from the given scope. @c offsetID will be added to all variable IDs before inserting them into the scope.
     * @return The number of variables copied.
     */
    int copyFromScope(Scope *scope, uint8_t offsetID);
    
    /** Whether the top scopes shall be accessable. */
    bool stop;
private:
    /**
     * This map contains the variables and values.
     * @warning Do not change directly. Use @c setVariable, and @c getVariable.
     * @private
     */
    std::map<EmojicodeString, CompilerVariable*> map;
};

struct ScopeWrapper {
    struct ScopeWrapper *topScope;
    Scope *scope;
};

/** Pops current scope and returns it. */
Scope* popScope();

/** Push this scope to be the current scope. */
void pushScope(Scope *scope);

/**
 * Retrieves a variable.
 */
CompilerVariable* getVariable(Token *variable, uint8_t *scopesUp);

/**
 * Sets a variable.
 * All scopes will be searched, if the variable was set in a top scope before it will receive the new value. If the variable was not set before it will be set in the current scope.
 */
void setVariable(Token *variable, CompilerVariable *value);


#endif /* defined(__Emojicode__Scope__) */
