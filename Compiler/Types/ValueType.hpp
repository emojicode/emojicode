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

namespace EmojicodeCompiler {

class ValueType : public TypeDefinition {
public:
    ValueType(std::u32string name, Package *p, SourcePosition pos, const std::u32string &documentation, bool exported,
              bool primitive);

    Type type() override { return Type(this); }

    bool canResolve(TypeDefinition *resolutionConstraint) const override {
        return resolutionConstraint == this;
    }

    void addInstanceVariable(const InstanceVariableDeclaration &declaration) override {
        if (primitive_) {
            throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
        }
        TypeDefinition::addInstanceVariable(declaration);
    }

    bool isPrimitive() const { return primitive_; }

    /// Whether this Value Type has a deinitializer and a copy retainer that must be called to deinitialize 
    bool isManaged();

    void setCopyRetain(llvm::Function *function) { copyRetain_ = function; }
    llvm::Function* copyRetain() { return copyRetain_; }

    void setBoxInfo(llvm::GlobalVariable *boxInfo) { boxInfo_ = boxInfo; }
    llvm::GlobalVariable* boxInfo() { return boxInfo_; }

    bool storesGenericArgs() const override;

    bool canInitFrom(const Type &literal) const override { return literal.type() == constructibleFrom_; }

    TypeType constructibleFrom_ = TypeType::NoReturn;

    virtual ~ValueType();

private:
    enum class Managed { Unknown, Yes, No };
    bool primitive_;
    Managed managed_ = Managed::Unknown;
    llvm::Function *copyRetain_ = nullptr;
    llvm::GlobalVariable *boxInfo_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* ValueType_hpp */
