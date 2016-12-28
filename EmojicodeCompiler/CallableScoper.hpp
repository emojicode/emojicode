//
//  CallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableScoper_hpp
#define CallableScoper_hpp

#include <list>
#include <utility>
#include "EmojicodeCompiler.hpp"
#include "Variable.hpp"
#include "Scoper.hpp"

/** Manages the scoping of a callable. */
class CallableScoper : public Scoper {
public:
    CallableScoper() {};
    CallableScoper(Scope *objectScope) : objectScope_(objectScope) {};
    
    /**
     * Retrieves a variable or throws a @c VariableNotFoundErrorException if the variable is not found.
     * @returns A pair: The variable and whether the variable was found in the object scope.
     */
    virtual std::pair<Variable&, bool> getVariable(const EmojicodeString &name, SourcePosition errorPosition);

    /** Returns the current sub scope. */
    Scope& currentScope();
    
    /** Pushes a new subscope and returns a reference to it. */
    Scope& pushScope();

    /** Pops the current scope and calls @c recommendFrozenVariables on it. */
    void popScopeAndRecommendFrozenVariables();
    
    /** Returns the object scope in which this callable operates or @c nullptr. */
    Scope* objectScope() const { return objectScope_; }

    Scope& topmostLocalScope();
private:
    std::list<Scope> scopes_;
    Scope *objectScope_ = nullptr;
};

#endif /* CallableScoper_hpp */
