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
    CallableScoper(Scope *instanceScope) : instanceScope_(instanceScope) {};
    
    /**
     * Retrieves a variable or throws a @c VariableNotFoundErrorException if the variable is not found.
     * @returns A pair: The variable and whether the variable was found in the instance scope.
     */
    virtual std::pair<Variable&, bool> getVariable(const EmojicodeString &name, SourcePosition errorPosition);

    /** Returns the current sub scope. */
    Scope& currentScope();
    
    /** Pushes a new subscope and returns a reference to it. */
    Scope& pushScope();

    /** Pops the current scope and calls @c recommendFrozenVariables on it. */
    void popScopeAndRecommendFrozenVariables();
    
    /** Returns the object scope in which this callable operates or @c nullptr. */
    Scope* instanceScope() const { return instanceScope_; }

    Scope& topmostLocalScope();
private:
    std::list<Scope> scopes_;
    Scope *instanceScope_ = nullptr;
};

#endif /* CallableScoper_hpp */
