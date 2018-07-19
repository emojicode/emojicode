//
//  ValueType.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef ValueType_hpp
#define ValueType_hpp

#include "TypeDefinition.hpp"
#include <utility>
#include <vector>

namespace llvm {
class GlobalVariable;
}  // namespace llvm

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation, bool exported)
        : TypeDefinition(std::move(name), p, std::move(pos), documentation, exported) {}

    Type type() override { return Type(this); }

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }

    void addInstanceVariable(const InstanceVariableDeclaration &declaration) override {
        if (primitive_) {
            throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
        }
        TypeDefinition::addInstanceVariable(declaration);
    }

    void makePrimitive() { primitive_ = true; }
    bool isPrimitive() { return primitive_; }

    void setBoxInfo(llvm::GlobalVariable *boxInfo) { boxInfo_ = boxInfo; }
    llvm::GlobalVariable* boxInfo() { return boxInfo_; }
private:
    bool primitive_ = false;
    llvm::GlobalVariable *boxInfo_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
