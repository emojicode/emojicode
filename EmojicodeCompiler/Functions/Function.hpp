//
//  Function.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Function_hpp
#define Function_hpp

#include "../Types/Generic.hpp"
#include "../Types/Type.hpp"
#include "../Types/TypeContext.hpp"
#include "FunctionType.hpp"
#include <experimental/optional>
#include <memory>
#include <utility>
#include <vector>

namespace llvm {
class Function;
class FunctionType;
}  // namespace llvm

namespace EmojicodeCompiler {

class ASTBlock;

enum class AccessLevel {
    Public, Private, Protected
};

struct Argument {
    Argument(std::u32string n, Type t) : variableName(std::move(n)), type(std::move(t)) {}

    /// The name of the variable
    std::u32string variableName;
    /// The type
    Type type;
};

/** Functions are callables that belong to a class or value type as either method, type method or initializer. */
class Function : public Generic<Function> {
public:
    Function(std::u32string name, AccessLevel level, bool final, Type owningType, Package *package, SourcePosition p,
             bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, FunctionType type)
    : position_(std::move(p)), name_(std::move(name)), final_(final), overriding_(overriding), deprecated_(deprecated), mutating_(mutating),
    access_(level), owningType_(std::move(owningType)), package_(package), documentation_(std::move(documentationToken)),
    functionType_(type) {}

    std::u32string name() const { return name_; }

    std::u32string protocolBoxingLayerName(const std::u32string &protocolName) {
        return protocolName + name()[0];
    }

    bool isExternal() const { return !externalName_.empty(); }
    const std::string& externalName() const { return externalName_; }
    void setExternalName(const std::string &name) { externalName_ = name; }

    /** Whether the method was marked as final and can’t be overriden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }
    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }

    std::vector<Argument> arguments;
    /** Return type of the method */
    Type returnType = Type::noReturn();

    /** Returns the position at which this callable was defined. */
    const SourcePosition& position() const { return position_; }

    /// The type of this function when used as value.
    Type type() const;

    /** Type to which this function belongs.
     This can be Nothingness if the function doesn’t belong to any type (e.g. 🏁). */
    const Type& owningType() const { return owningType_; }
    void setOwningType(const Type &type) { owningType_ = type; }

    const std::u32string& documentation() const { return documentation_; }

    /// The package in which the function was defined.
    /// This does not necessarily match the package of @c owningType.
    Package* package() const { return package_; }

    /// Issues a warning at the given position if the function is deprecated.
    void deprecatedWarning(const SourcePosition &p) const;

    /// Checks that no promises were broken and applies boxing if necessary.
    /// @returns false iff a value for protocol was given and the arguments or the return type are storage incompatible.
    /// This indicates that a BoxingLayer must be created.
    bool enforcePromises(Function *super, const TypeContext &typeContext, const Type &superSource,
                         std::experimental::optional<TypeContext> protocol);

    void registerOverrider(Function *f) { overriders_.push_back(f); }

    /// @returns True iff the function was assigned a virtual table index.
    bool hasVti() const { return vti_ > -1; }
    /// Sets the VTI to the given value.
    void setVti(int vti);
    /// Returns the VTI this function was assigned.
    int getVti() const;

    TypeContext typeContext() {
        auto type = owningType();
        if (type.type() == TypeType::ValueType || type.type() == TypeType::Enum) {
            type.setReference();
        }
        return TypeContext(type, this);
    }

    /// Whether the function mutates the callee. Only relevant for value type instance methods.
    bool mutating() const { return mutating_; }

    FunctionType functionType() const { return functionType_; }
    void setFunctionType(FunctionType type) { functionType_ = type; }

    virtual ContextType contextType() const {
        switch (functionType()) {
            case FunctionType::ObjectMethod:
            case FunctionType::ObjectInitializer:
                return ContextType::Object;
                break;
            case FunctionType::ValueTypeMethod:
            case FunctionType::ValueTypeInitializer:
                return ContextType::ValueReference;
                break;
            case FunctionType::ClassMethod:
            case FunctionType::Function:
                return ContextType::None;
                break;
            case FunctionType::BoxingLayer:
                throw std::logic_error("contextType for BoxingLayer called on Function class");
        }
    }

    void setAst(const std::shared_ptr<ASTBlock> &ast) { ast_ = ast; }
    const std::shared_ptr<ASTBlock>& ast() const { return ast_; }

    llvm::Function* llvmFunction() const { return llvmFunction_; }
    virtual llvm::FunctionType* llvmFunctionType() const;
    void setLlvmFunction(llvm::Function *lf) { llvmFunction_ = lf; }

    size_t variableCount() const { return variableCount_; }
    void setVariableCount(size_t variableCount) { variableCount_ = variableCount; }
protected:
    std::vector<Function*> overriders_;
    bool used_ = false;
private:
    std::shared_ptr<ASTBlock> ast_;
    SourcePosition position_;
    std::u32string name_;
    int vti_ = -1;

    bool final_;
    bool overriding_;
    bool deprecated_;
    bool mutating_;

    std::string externalName_;
    AccessLevel access_;
    Type owningType_;
    Package *package_;
    std::u32string documentation_;
    FunctionType functionType_;
    size_t variableCount_ = 0;

    llvm::Function *llvmFunction_;
};

}  // namespace EmojicodeCompiler

#endif /* Function_hpp */
