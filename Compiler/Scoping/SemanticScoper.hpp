//
//  CallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CallableScoper_hpp
#define CallableScoper_hpp

#include "Functions/Function.hpp"
#include "Scope.hpp"
#include <list>
#include <vector>

namespace EmojicodeCompiler {

struct ResolvedVariable {
    ResolvedVariable(Variable &variable, bool inInstanceScope) : variable(variable), inInstanceScope(inInstanceScope) {}
    Variable &variable;
    bool inInstanceScope;
};

struct FunctionObjectVariableInformation;

/** Manages the scoping of a callable. */
class SemanticScoper {
public:
    SemanticScoper() = default;
    explicit SemanticScoper(Scope *instanceScope) : instanceScope_(instanceScope) {};

    static SemanticScoper scoperForFunction(Function *function);

    /// Retrieves a variable or throws a @c VariableNotFoundError if the variable is not found.
    virtual ResolvedVariable getVariable(const std::u32string &name, const SourcePosition &errorPosition);

    /// Returns the current subscope.
    Scope& currentScope() {
        return scopes_.front();
    }

    /// Pushes a new subscope and returns a reference to it.
    Scope& pushScope();

    /// Pushes a new subscope and sets the argument variables in it.
    virtual Scope& pushArgumentsScope(const std::vector<Parameter> &arguments, const SourcePosition &p);

    /// Pops the current scope and calls @c recommendFrozenVariables on it.
    void popScope(Compiler *app);

    /// Returns the instance scope or @c nullptr
    Scope* instanceScope() const { return instanceScope_; }

    /// Returns the topmost local scope, i.e. the one in which all other locals scopes are subscopes.
    Scope& topmostLocalScope() {
        return scopes_.back();
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
    int maxInitializationLevel_ = 1;
    size_t maxVariableId_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* CallableScoper_hpp */
