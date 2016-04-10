//
//  CompilerScope.h
//  Emojicode
//
//  Created by Theo Weidmann on 06.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef CompilerScope_hpp
#define CompilerScope_hpp

#include <forward_list>
#include "EmojicodeCompiler.hpp"

class CompilerVariable {
public:
    CompilerVariable(Type type, uint8_t id, bool initd, bool frozen, const Token *token)
        : type(type), id(id), initialized(initd), definitionToken(token), frozen_(frozen) {};
    /** The type of the variable. */
    Type type;
    /** The ID of the variable. */
    uint8_t id;
    /** The variable is initialized if this field is greater than 0. */
    int initialized;
    /** The variable name token which defined this variable. */
    const Token *definitionToken;
    
    /** Throws an error if the variable is not initalized. */
    void uninitalizedError(const Token *variableToken) const;
    /** Marks the variable as mutated or issues an error if the variable is frozen. */
    void mutate(const Token *variableToken);
    
    bool mutated() { return mutated_; }
    bool frozen() { return frozen_; }
private:
    /** Indicating whether variable was frozen. */
    bool frozen_;
    /** Mutated */
    bool mutated_ = false;
};

/** A variable scope */
class Scope {
public:
    Scope(bool stop) : stop(stop) {};
    ~Scope();
    void changeInitializedBy(int c);
    
    /** Sets a variable in this scope. */
    void setLocalVariable(const Token *variable, CompilerVariable *value);
    void setLocalVariable(EmojicodeString string, CompilerVariable *value);
    
    /**
     * Retrieves a variable form the scope or returns @c nullptr.
     */
    CompilerVariable* getLocalVariable(const Token *variable);
    
    /** 
     * Emits @c errorMessage if not all instance variable were initialized.
     * @params errorMessage The error message that will probably be issued. It should include @c %s for the name of the variable.
     */
    void initializerUnintializedVariablesCheck(const Token *errorToken, const char *errorMessage);
    
    /**
     * Emits a warning for each non-frozen variable that has not been mutated.
     */
    void recommendFrozenVariables();
    
    /**
     * Copies the variable from the given scope. @c offsetID will be added to all variable IDs before inserting them into the scope.
     * @return The number of variables copied.
     */
    int copyFromScope(Scope *scope, uint8_t offsetID);
    
    /** Whether the top scopes shall be accessable. */
    bool stop;
private:
    std::map<EmojicodeString, CompilerVariable*> map;
};

class Scoper {
public:
    /**
     * Retrieves a variable.
     */
    CompilerVariable* getVariable(const Token *variable, uint8_t *scopesUp);
    
    /**
     * Sets a variable.
     * All scopes will be searched, if the variable was set in a top scope before it will receive the new value. If the variable was not set before it will be set in the current scope.
     */
    void setVariable(const Token *variable, CompilerVariable *value);
    
    /** Returns the current scope */
    Scope* currentScope();
    
    /** Returns the top scope of this scope. */
    Scope* topScope();
    
    void popScope();
    void pushScope(Scope *);
private:
    std::forward_list<Scope*> scopes;
};

#endif /* CompilerScope_hpp */
