//
//  Scope.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Scope_hpp
#define Scope_hpp

#include "../Lex/SourcePosition.hpp"
#include "Variable.hpp"
#include "VariableID.hpp"
#include <map>

namespace EmojicodeCompiler {

class Scope {
public:
    explicit Scope(unsigned int variableId) : maxVariableId_(variableId) {}

    void setVariableInitialization(bool initd);
    void pushInitializationLevel();
    void popInitializationLevel();

    /// Sets a variable in this scope and returns it.
    /// @throws CompilerError if a variable with this name already exists.
    Variable& declareVariable(const EmojicodeString &variable, const Type &type, bool frozen, const SourcePosition &p);

    /// Sets a variable with the given ID in this scope and returns it.
    /// @throws CompilerError if a variable with this name already exists.
    Variable& declareVariableWithId(const EmojicodeString &variable, const Type &type, bool frozen, VariableID id,
                                    const SourcePosition &p);

    /// Creates an internal variable for the given type. It will be marked as mutated and non-frozen.
    Variable& declareInternalVariable(const Type &type, const SourcePosition &p) {
        auto &var = declareVariable(internalName(), type, false, p);
        var.mutate(p);
        return var;
    }

    /// Retrieves a variable form the scope. Use @c hasLocalVariable to determine whether the variable with this name
    /// is in this scope.
    Variable& getLocalVariable(const EmojicodeString &varable);
    /// Returns true if a variable with the name @c variable is set in this scope.
    bool hasLocalVariable(const EmojicodeString &variable) const;

    /**
     * Emits @c errorMessage if not all instance variable were initialized.
     * @params errorMessage The error message that will probably be issued. It should include @c %s for the name
     of the variable.
     */
    void initializerUnintializedVariablesCheck(const SourcePosition &p, const char *errorMessage);

    /// Emits a warning for each non-frozen variable that has not been mutated.
    void recommendFrozenVariables() const;

    const std::map<EmojicodeString, Variable>& map() const { return map_; }

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
    /// Returns a name for an internal variable
    EmojicodeString internalName() {
        auto string = EmojicodeString(E_LOCK);
        auto number = std::to_string(internalCount_++);
        string.resize(number.size() + 1);
        auto stringData = &string[1];
        for (auto numberChar : number) {
            *stringData++ = numberChar;
        }
        return string;
    }

    std::map<EmojicodeString, Variable> map_;
    int internalCount_ = 0;
    unsigned int maxVariableId_;
};

}  // namespace EmojicodeCompiler

#endif /* Scope_hpp */
