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

class Variable;
class Type;
class Compiler;
class PathAnalyser;
struct SourcePosition;

class Scope {
public:
    Scope() = default;
    explicit Scope(unsigned int id) : maxVariableId_(id) {}

    /// Sets a variable in this scope and returns it.
    /// @throws CompilerError if a variable with this name already exists.
    Variable& declareVariable(const std::u32string &variable, const Type &type, bool constant, const SourcePosition &p);
    /// Sets a variable with the given ID in this scope and returns it.
    /// @throws CompilerError if a variable with this name already exists.
    Variable& declareVariableWithId(const std::u32string &variable,  Type type, bool constant, VariableID id,
                                    const SourcePosition &p);

    /// Retrieves a variable form the scope. Use @c hasLocalVariable to determine whether the variable with this name
    /// is in this scope.
    Variable& getLocalVariable(const std::u32string &variable);
    /// Returns true if a variable with the name @c variable is set in this scope.
    bool hasLocalVariable(const std::u32string &variable) const;

    /// Emits a warning for each mutable variable that has not been mutated.
    /// Ensures that all non-optional variables are either initialized or are certainly not.
    void checkScope(PathAnalyser *analyser, Compiler *compiler) const;

    void markInherited() {
        for (auto &pair : map_) {
            pair.second.setInherited();
        }
    }

    std::map<std::u32string, Variable>& map() { return map_; }
    const std::map<std::u32string, Variable>& map() const { return map_; }

    unsigned int maxVariableId() const { return maxVariableId_; }
    unsigned int reserveIds(unsigned int count) {
        auto id = maxVariableId();
        maxVariableId_ += count;
        return id;
    }

private:
    std::map<std::u32string, Variable> map_;
    unsigned int maxVariableId_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* Scope_hpp */
