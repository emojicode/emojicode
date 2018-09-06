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
#include <memory>

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation, bool exported);

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
    bool isPrimitive() const { return primitive_; }

    /// Whether this Value Type has a deinitializer and a copy retainer that must be called to deinitialize 
    bool isManaged();

    Function* copyRetain();

    void setBoxInfo(llvm::GlobalVariable *boxInfo) { boxInfo_ = boxInfo; }
    llvm::GlobalVariable* boxInfo() { return boxInfo_; }

    virtual ~ValueType();

private:
    enum class Managed { Unknown, Yes, No };
    bool primitive_ = false;
    Managed managed_ = Managed::Unknown;
    std::unique_ptr<Function> copyRetain_;
    llvm::GlobalVariable *boxInfo_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
