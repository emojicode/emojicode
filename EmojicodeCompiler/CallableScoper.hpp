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
#include <vector>
#include "Scoper.hpp"
#include "EmojicodeCompiler.hpp"
#include "Variable.hpp"

namespace EmojicodeCompiler {

struct ResolvedVariable {
    ResolvedVariable(Variable &variable, bool inInstanceScope) : variable(variable), inInstanceScope(inInstanceScope) {}
    Variable &variable;
    bool inInstanceScope;
};

struct FunctionObjectVariableInformation;

/** Manages the scoping of a callable. */
class CallableScoper : public Scoper {
public:
    CallableScoper() {};
    CallableScoper(const Scoper&) = delete;
    explicit CallableScoper(Scope *instanceScope) : instanceScope_(instanceScope) {};

    /// Retrieves a variable or throws a @c VariableNotFoundError if the variable is not found.
    virtual ResolvedVariable getVariable(const EmojicodeString &name, const SourcePosition &errorPosition);

    /// Returns the current subscope.
    Scope& currentScope() {
        return scopes_.front();
    }

    /// Pushes a new subscope and returns a reference to it.
    Scope& pushScope();

    /// Pops the current scope and calls @c recommendFrozenVariables on it.
    void popScope(InstructionCount count);

    /// Returns the instance scope or @c nullptr
    Scope* instanceScope() const { return instanceScope_; }

    /// Returns the topmost local scope, i.e. the one in which all other locals scopes are subscopes.
    Scope& topmostLocalScope() {
        return scopes_.back();
    }

    /// This method is called by the FunctionPAG after all arguments were set
    virtual void postArgumentsHook() {}

    Scope& pushTemporaryScope() {
        if (pushedTemporaryScope_) {
            return currentScope();
        }
        scopes_.emplace_front(Scope(this));
        pushedTemporaryScope_ = true;
        return currentScope();
    }

    void clearTemporaryScope() {
        if (!pushedTemporaryScope_) {
            return;
        }
        pushedTemporaryScope_ = false;
        reduceOffsetBy(static_cast<int>(scopes_.front().size()));
        scopes_.pop_front();
    }

    void setVariableInformation(std::vector<FunctionObjectVariableInformation> *info) {
        info_ = info;
    }
protected:
    int maxInitializationLevel() const { return maxInitializationLevel_; }
private:
    std::list<Scope> scopes_;
    Scope *instanceScope_ = nullptr;
    bool pushedTemporaryScope_ = false;
    int maxInitializationLevel_ = 0;
    std::vector<FunctionObjectVariableInformation> *info_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* CallableScoper_hpp */
