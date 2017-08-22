//
//  CallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableScoper_hpp
#define CallableScoper_hpp

#include "../EmojicodeCompiler.hpp"
#include "../Function.hpp"
#include <list>
#include <vector>

namespace EmojicodeCompiler {

struct ResolvedVariable {
    ResolvedVariable(Variable &variable, bool inInstanceScope) : variable(variable), inInstanceScope(inInstanceScope) {}
    Variable &variable;
    bool inInstanceScope;
};

struct FunctionObjectVariableInformation;
class Function;

/** Manages the scoping of a callable. */
class SemanticScoper {
public:
    SemanticScoper() = default;
    explicit SemanticScoper(Scope *instanceScope) : instanceScope_(instanceScope) {};

    static SemanticScoper scoperForFunction(Function *function);

    /// Retrieves a variable or throws a @c VariableNotFoundError if the variable is not found.
    virtual ResolvedVariable getVariable(const EmojicodeString &name, const SourcePosition &errorPosition);

    /// Returns the current subscope.
    Scope& currentScope() {
        return scopes_.front();
    }

    /// Pushes a new subscope and returns a reference to it.
    Scope& pushScope();

    /// Pushes a new subscope and sets the argument variables in it.
    virtual Scope& pushArgumentsScope(const std::vector<Argument> &arguments, const SourcePosition &p);

    /// Pops the current scope and calls @c recommendFrozenVariables on it.
    void popScope();

    /// Returns the instance scope or @c nullptr
    Scope* instanceScope() const { return instanceScope_; }

    /// Returns the topmost local scope, i.e. the one in which all other locals scopes are subscopes.
    Scope& topmostLocalScope() {
        return scopes_.back();
    }

    /// Pushes the temporary scope if it has not been pushed yet. No other scope may be pushed upon this scope.
    /// The temporary scope is used to temporarily store value type i.a. instances that have no stack location to obtain
    /// a reference to them.
    Scope& pushTemporaryScope() {
        if (pushedTemporaryScope_) {
            return currentScope();
        }
        pushScopeInternal();
        pushedTemporaryScope_ = true;
        return currentScope();
    }

    /// Pops the temporary scope, if it was pushed.
    /// @returns True if the scope had been pushed and was poped.
    bool popTemporaryScope() {
        if (!pushedTemporaryScope_) {
            return false;
        }
        pushedTemporaryScope_ = false;
        updateMaxVariableIdForPopping();
        scopes_.pop_front();
        return true;
    }

    /// The number of variable ids that were assigned.
    size_t variableIdCount() const { return maxVariableId_; }
protected:
    int maxInitializationLevel() const { return maxInitializationLevel_; }
private:
    void updateMaxVariableIdForPopping() {
        auto &scope = currentScope();
        if (scope.maxVariableId() > maxVariableId_) {
            maxVariableId_ = scope.maxVariableId();
        }
    }

    void pushScopeInternal() {
        scopes_.emplace_front(Scope(scopes_.empty() ? maxVariableId_ : scopes_.front().maxVariableId()));
    }

    std::list<Scope> scopes_;
    Scope *instanceScope_ = nullptr;
    bool pushedTemporaryScope_ = false;
    int maxInitializationLevel_ = 1;
    size_t maxVariableId_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* CallableScoper_hpp */
