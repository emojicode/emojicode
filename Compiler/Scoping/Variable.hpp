//
//  Variable.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Variable_hpp
#define Variable_hpp

#include <utility>

#include "Lex/SourcePosition.hpp"
#include "Types/Type.hpp"

namespace EmojicodeCompiler {

class CapturingSemanticScoper;

using VariableID = size_t;

class Variable {
    friend CapturingSemanticScoper;
public:
    Variable(Type type, VariableID id, bool constant, std::u32string string, SourcePosition p)
        : type_(std::move(type)), constant_(constant), string_(std::move(string)), id_(id), position_(std::move(p)) {}
    /// The type of the variable.
    const Type type() const { return type_; }

    /// The name of this variable.
    const std::u32string& name() const { return string_; }

    /// The position at which this variable was defined
    const SourcePosition& position() const { return position_; }

    /// Marks the variable as mutated or issues an error if the variable is constant.
    /// @throws CompilerError if the variable is constant.
    void mutate(const SourcePosition &p);

    /// Whether the variable was mutated since its definition.
    bool mutated() const { return mutated_; }

    /// Whether this is a constant variable.
    bool constant() const { return constant_; }

    bool inherited() const { return inherited_; }
    void setInherited() { inherited_ = true; }

    VariableID id() const { return id_; }
private:
    Type type_;
    bool constant_;
    bool mutated_ = false;
    bool inherited_ = false;
    std::u32string string_;
    VariableID id_;

    SourcePosition position_;
};

}  // namespace EmojicodeCompiler

#endif /* Variable_hpp */
