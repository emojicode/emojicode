//
//  Function.h
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Function_hpp
#define Function_hpp

#include "FunctionType.hpp"
#include "Types/Generic.hpp"
#include "AST/ASTType.hpp"
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

struct Parameter {
    Parameter(std::u32string n, std::unique_ptr<ASTType> t) : name(std::move(n)), type(std::move(t)) {}

    /// The name of the variable
    std::u32string name;
    /// The type
    std::shared_ptr<ASTType> type;
};

class FunctionReification {
public:
    llvm::Function *function = nullptr;
    unsigned int vti() { return vti_; }
    void setVti(unsigned int vti) { vti_ = vti; }
    llvm::FunctionType* functionType();
    void setFunctionType(llvm::FunctionType *type) { functionType_ = type; }
private:
    llvm::FunctionType* functionType_ = nullptr;
    unsigned int vti_ = 0;
};

/** Functions are callables that belong to a class or value type as either method, type method or initializer. */
class Function : public Generic<Function, FunctionReification> {
public:
    Function() = delete;
    Function(std::u32string name, AccessLevel level, bool final, TypeDefinition *owner, Package *package,
             SourcePosition p,
             bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, bool imperative,
             bool unsafe, FunctionType type);

    std::u32string name() const { return name_; }

    std::u32string protocolBoxingThunk(const std::u32string &protocolName) const {
        return protocolName + name()[0];
    }

    bool isExternal() const { return external_; }
    const std::string& externalName() const { return externalName_; }
    void makeExternal() { external_ = true; }
    void setExternalName(const std::string &name) {
        external_ = true;
        externalName_ = name;
    }

    /** Whether the method was marked as final and can’t be overridden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }

    bool unsafe() const { return unsafe_; }

    bool isImperative() const { return imperative_; }
    /// Whether the function mutates the callee. Only relevant for value type instance methods.
    bool mutating() const { return mutating_; }

    void setMutating(bool v) { mutating_ = v; }

    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }

    const std::vector<Parameter>& parameters() const { return parameters_; }

    void setParameters(std::vector<Parameter> parameters) { parameters_ = std::move(parameters); }

    void setParameterType(size_t index, std::unique_ptr<ASTType> type) { parameters_[index].type = std::move(type); }

    ASTType* returnType() const { return returnType_.get(); }

    void setReturnType(std::unique_ptr<ASTType> type) { returnType_ = std::move(type); }

    /// Returns the position at which this function was defined.
    const SourcePosition& position() const { return position_; }

    /// The type definition in which this function was defined.
    /// @returns nullptr if the function does not belong to a type (is not a method or initializer).
    TypeDefinition* owner() const { return owner_; }
    void setOwner(TypeDefinition *typeDef) { owner_ = typeDef; }

    const std::u32string& documentation() const { return documentation_; }

    /// The package in which the function was defined.
    /// This does not necessarily match the package of @c owningType.
    Package* package() const { return package_; }

    /// @see setVirtualTableThunk()
    Function* virtualTableThunk() const { return virtualTableThunk_; };
    /// The provided thunk function is placed at the designated virtual table index instead of the this function.
    /// This function is assigned another virtual table index.
    void setVirtualTableThunk(Function *layer) { virtualTableThunk_ = layer; }

    TypeContext typeContext();

    FunctionType functionType() const { return functionType_; }

    void setAst(std::unique_ptr<ASTBlock> ast);
    ASTBlock* ast() const { return ast_.get(); }

    size_t variableCount() const { return variableCount_; }
    void setVariableCount(size_t variableCount) { variableCount_ = variableCount; }

    virtual ~Function();

private:
    std::vector<Parameter> parameters_;
    std::unique_ptr<ASTType> returnType_;

    std::unique_ptr<ASTBlock> ast_;
    SourcePosition position_;
    std::u32string name_;

    bool final_;
    bool overriding_;
    bool deprecated_;
    bool imperative_;
    bool unsafe_;

    bool mutating_;
    bool external_ = false;

    Function *virtualTableThunk_ = nullptr;

    std::string externalName_;
    AccessLevel access_;
    TypeDefinition *owner_;
    Package *package_;
    std::u32string documentation_;
    FunctionType functionType_;
    size_t variableCount_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* Function_hpp */
