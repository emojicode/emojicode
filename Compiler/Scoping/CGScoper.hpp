//
//  CGScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef CGScoper_hpp
#define CGScoper_hpp

#include <vector>
#include <cstddef>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

/// LocalVariable instances represent variables in a CGScoper.
struct LocalVariable {
    LocalVariable() = default;
    LocalVariable(bool isMutable, llvm::Value *value) : isMutable(isMutable), value(value) {}
    /// If this field is true, ::value is a pointer to stack memory allocated by the `alloca` instruction. Otherwise
    /// ::value can be the result of any instructions.
    bool isMutable = true;
    /// The value that is currently represented by this variable.
    /// @see isMutable
    llvm::Value *value = nullptr;
};

/// CGScoper is used to keep track of the llvm::Value instances that are currently assigned to local variables during
/// their generation with FunctionCodeGenerator.
///
/// CGScoper is only a thin layer over a vector. It simply and strictly works like a table and relies on that
/// SemanticScoper correctly issues VariableIDs, which are the indices by which the vector is accessed. Variables can
/// therefore be retrieved in constant time. No real scoping meachnics are provided by this class, all scoping
/// mechanics are implemented in SemanticScoper, which issues ids so that rows of a CGScoper are reused once the
/// corresponding variable dropped out of scope.
class CGScoper {
public:
    explicit CGScoper(size_t variables) : variables_(variables, LocalVariable()) {}

    void resizeVariables(size_t to) {
        variables_.resize(to, LocalVariable());
    }

    /// Gets the variable with the given ID.
    /// @complexity O(1)
    LocalVariable& getVariable(size_t id) {
        return variables_[id];
    }
private:
    std::vector<LocalVariable> variables_;
};

}  // namespace EmojicodeCompiler

#endif /* CGScoper_hpp */
