//
//  CallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableScoper_hpp
#define CallableScoper_hpp

#include <forward_list>
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
    std::pair<Variable&, bool> getVariable(const EmojicodeString &name, SourcePosition errorPosition);

    /** Returns the current sub scope. */
    Scope& currentScope();
    
    /** Pushes a new subscope and returns a reference to it. */
    Scope& pushScope();

    /** Pops the current scope and calls @c recommendFrozenVariables on it. */
    void popScopeAndRecommendFrozenVariables();
    
    /**
     * Returns a copy of this scoper with all scopes (except the object scope) merged into a single scope. 
     * Variable IDs are preserved with @c argumentCount added.
     * @returns A pair: The copy of the scoper and
     */
//    std::pair<CallableScoper, int> flattenedCopy(int argumentCount) const;
    
    /** Returns the object scope in which this callable operates or @c nullptr. */
    Scope* objectScope() const { return objectScope_; }
private:
    std::forward_list<Scope> scopes_;
    Scope *objectScope_ = nullptr;
};

#endif /* CallableScoper_hpp */
