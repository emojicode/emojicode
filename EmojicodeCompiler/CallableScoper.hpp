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
#include "EmojicodeCompiler.hpp"
#include "Variable.hpp"
#include "Scoper.hpp"

struct ResolvedVariable {
    ResolvedVariable(Variable &variable, bool inInstanceScope) : variable(variable), inInstanceScope(inInstanceScope) {}
    Variable &variable;
    bool inInstanceScope;
};

/** Manages the scoping of a callable. */
class CallableScoper : public Scoper {
public:
    CallableScoper() {};
    explicit CallableScoper(Scope *instanceScope) : instanceScope_(instanceScope) {};

    /**
     * Retrieves a variable or throws a @c VariableNotFoundError if the variable is not found.
     * @returns A pair: The variable and whether the variable was found in the instance scope.
     */
    virtual ResolvedVariable getVariable(const EmojicodeString &name, SourcePosition errorPosition);

    /** Returns the current sub scope. */
    Scope& currentScope();

    /** Pushes a new subscope and returns a reference to it. */
    Scope& pushScope();

    /** Pops the current scope and calls @c recommendFrozenVariables on it. */
    void popScopeAndRecommendFrozenVariables();

    /** Returns the object scope in which this callable operates or @c nullptr. */
    Scope* instanceScope() const { return instanceScope_; }

    Scope& topmostLocalScope();

    void pushTemporaryScope() { /* pushScope(); temporaryScopes_++; */ }
    void popTemporaryScope() {
        if (temporaryScopes_) {
            popScopeAndRecommendFrozenVariables();
            temporaryScopes_--;
        }
    }
    void clearAllTemporaryScopes() {
        if (temporaryScopes_ > 0) throw std::logic_error("Not all temporary scoeps popped");
    }
private:
    std::list<Scope> scopes_;
    Scope *instanceScope_ = nullptr;
    int temporaryScopes_ = 0;
};

#endif /* CallableScoper_hpp */
