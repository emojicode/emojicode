//
//  Scope.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Scope_hpp
#define Scope_hpp

#include "Variable.hpp"
#include <map>

namespace EmojicodeCompiler {

class Application;
struct SourcePosition;

class Scope {
public:
    explicit Scope(unsigned int variableId) : maxVariableId_(variableId) {}

    void setVariableInitialization(bool initd);
    void pushInitializationLevel();
    void popInitializationLevel();

    /// Sets a variable in this scope and returns it.
    /// @throws CompilerError if a variable with this name already exists.
    Variable& declareVariable(const std::u32string &variable, const Type &type, bool frozen, const SourcePosition &p);

    /// Retrieves a variable form the scope. Use @c hasLocalVariable to determine whether the variable with this name
    /// is in this scope.
    Variable& getLocalVariable(const std::u32string &variable);
    /// Returns true if a variable with the name @c variable is set in this scope.
    bool hasLocalVariable(const std::u32string &variable) const;

    /// Throws an error if an uninitialized variable is found in this scope.
    /// The error message is created by concatenating errorMessageFront, the variable name, and errorMessageBack.
    void unintializedVariablesCheck(const SourcePosition &p, const std::string &errorMessageFront,
                                    const std::string &errorMessageBack);

    /// Emits a warning for each non-frozen variable that has not been mutated.
    void recommendFrozenVariables(Application *app) const;

    const std::map<std::u32string, Variable>& map() const { return map_; }

    void markInherited() {
        for (auto &pair : map_) {
            pair.second.setInherited();
        }
    }

    unsigned int maxVariableId() const { return maxVariableId_; }
    unsigned int reserveIds(unsigned int count) {
        auto id = maxVariableId();
        maxVariableId_ += count;
        return id;
    }
private:
    std::map<std::u32string, Variable> map_;
    int internalCount_ = 0;
    unsigned int maxVariableId_;
};

}  // namespace EmojicodeCompiler

#endif /* Scope_hpp */
