//
//  IDScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef IDScoper_hpp
#define IDScoper_hpp

#include <cstddef>
#include <vector>

namespace llvm {
class Value;
}  // namespace llvm

namespace EmojicodeCompiler {

/// LocalVariable instances represent variables in an CGScoper.
struct LocalVariable {
    LocalVariable() = default;
    LocalVariable(bool isMutable, llvm::Value* value) : isMutable(isMutable), value(value) {}
    /// If this field is true, ::value is a pointer to stack memory allocated by the `alloca` instruction. Otherwise
    /// ::value can be the result of any instructions.
    bool isMutable = true;
    /// The value that is currently represented by this variable.
    /// @see isMutable
    llvm::Value* value = nullptr;
};


/// IDScoper is only a thin layer over a vector. It simply and strictly works like a table and relies on that
/// SemanticScoper correctly issues VariableIDs, which are the indices by which the vector is accessed. Variables can
/// therefore be retrieved in constant time. No real scoping mechanics are provided by this class, all scoping
/// is implemented in SemanticScoper, which issues IDs so that rows of a IDScoper are reused once the
/// corresponding variable dropped out of scope.
template <typename Element>
class IDScoper {
public:
    explicit IDScoper(size_t variables) : variables_(variables, Element()) {}

    /// Gets the variable with the given ID.
    /// @complexity O(1)
    Element& getVariable(size_t id) {
        return variables_[id];
    }
private:
    std::vector<Element> variables_;
};

/// CGScoper is used to keep track of the llvm::Value instances that are currently assigned to local variables during
/// their generation with FunctionCodeGenerator.
using CGScoper = IDScoper<LocalVariable>;

}  // namespace EmojicodeCompiler

#endif /* CGScoper_hpp */
