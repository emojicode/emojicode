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
#include "Mood.hpp"
#include "Types/Generic.hpp"
#include "MemoryFlowAnalysis/MFFlowCategory.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace llvm {
class Function;
class FunctionType;
}  // namespace llvm

namespace EmojicodeCompiler {

class ASTBlock;
class ASTType;

enum class AccessLevel {
    /// If none is specified. Either resolves to the overridden method’s AccessLevel or defaults to Public.
    Default,
    Public,
    Private,
    Protected
};

struct Parameter {
    Parameter(std::u32string n, std::unique_ptr<ASTType> t, MFFlowCategory mft)
        : name(std::move(n)), type(std::move(t)), memoryFlowType(mft) {}

    /// The name of the variable
    std::u32string name;
    /// The type
    std::shared_ptr<ASTType> type;

    MFFlowCategory memoryFlowType = MFFlowCategory::Borrowing;
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

/// Function represents any Function (i.e. type method, method, initializer, start flag function, deinitializer)
/// defined in code or automatically synthesized (e.g. deinitializer) by the compiler.
class Function : public Generic<Function, FunctionReification> {
public:
    Function() = delete;
    Function(std::u32string name, AccessLevel level, bool final, TypeDefinition *owner, Package *package,
             SourcePosition p,
             bool overriding, std::u32string documentationToken, bool deprecated, bool mutating, Mood mood,
             bool unsafe, FunctionType type, bool forceInline);

    /// The name of the function.
    std::u32string name() const { return name_; }

    std::u32string protocolBoxingThunk(const std::u32string &protocolName) const {
        return protocolName + name()[0];
    }

    /// True iff the function’s implementation is not provided.
    /// This can either mean that the function was imported from another package or that it does not have an
    /// implementation in Emojicode but is defined elsewhere (e.g. C++) and just linked against.
    /// @see externalName
    bool isExternal() const { return external_; }
    /// If this method returns a non-empty string it is the named that shall be used for the function when generating
    /// code.
    const std::string& externalName() const { return externalName_; }
    void makeExternal() { external_ = true; }
    /// Makes the function external and sets name as its external name.
    /// Pratically, this means that the function does not have a body and is justed linked against the specified
    /// name.
    void setExternalName(const std::string &name) {
        external_ = true;
        externalName_ = name;
    }

    /// The package in which the function was defined.
    /// This is not necessarily the package of owner().
    Package* package() const { return package_; }

    /// Returns the position at which this function was defined.
    const SourcePosition& position() const { return position_; }

    /// The type definition in which this function was defined.
    /// @returns nullptr if the function does not belong to a type (is not a method or initializer).
    TypeDefinition* owner() const { return owner_; }
    void setOwner(TypeDefinition *typeDef) { owner_ = typeDef; }

    const std::u32string& documentation() const { return documentation_; }

    /** Whether the method was marked as final and can’t be overridden. */
    bool final() const { return final_; }
    /** Whether the method is intended to override a super method. */
    bool overriding() const { return overriding_; }
    /** Whether the method is deprecated. */
    bool deprecated() const { return deprecated_; }

    bool unsafe() const { return unsafe_; }

    Mood mood() const { return mood_; }
    /// Whether the function mutates the callee. Only relevant for value type instance methods.
    bool mutating() const { return mutating_; }

    void setMutating(bool v) { mutating_ = v; }

    bool isInline() const;

    void setThunk() { thunk_ = true; }
    bool isThunk() const { return thunk_; }

    /** Returns the access level to this method. */
    AccessLevel accessLevel() const { return access_; }
    void setAccessLevel(AccessLevel level) { access_ = level; }

    const std::vector<Parameter>& parameters() const { return parameters_; }

    void setParameters(std::vector<Parameter> parameters) { parameters_ = std::move(parameters); }

    void setParameterType(size_t index, std::unique_ptr<ASTType> type) { parameters_[index].type = std::move(type); }
    void setParameterMFType(size_t index, MFFlowCategory type) { parameters_[index].memoryFlowType = type; }

    ASTType* returnType() const { return returnType_.get(); }

    void setReturnType(std::unique_ptr<ASTType> type) { returnType_ = std::move(type); }

    /// @see setVirtualTableThunk()
    Function* virtualTableThunk() const { return virtualTableThunk_; };
    /// The provided thunk function is placed at the designated virtual table index instead of the this function.
    /// This function is assigned another virtual table index.
    void setVirtualTableThunk(Function *layer) { virtualTableThunk_ = layer; }

    TypeContext typeContext();

    FunctionType functionType() const { return functionType_; }
    void setFunctionType(FunctionType functionType) { functionType_ = functionType; }

    void setAst(std::unique_ptr<ASTBlock> ast);
    ASTBlock* ast() const { return ast_.get(); }

    size_t variableCount() const { return variableCount_; }
    void setVariableCount(size_t variableCount) { variableCount_ = variableCount; }

    void setClosure() { closure_ = true; }
    bool isClosure() const { return closure_; }

    MFFlowCategory memoryFlowTypeForThis() const { return memoryFlowTypeThis_; }
    void setMemoryFlowTypeForThis(MFFlowCategory type) { memoryFlowTypeThis_ = type; }

    /// Whether this initializer might return an error.
    bool errorProne() const { return errorType_ != nullptr && errorType_->type().type() != TypeType::NoReturn; }
    ASTType* errorType() const { return errorType_.get(); }
    void setErrorType(std::unique_ptr<ASTType> type) { errorType_ = std::move(type); }

    virtual ~Function();

private:
    std::vector<Parameter> parameters_;
    std::unique_ptr<ASTType> returnType_;
    std::unique_ptr<ASTType> errorType_;

    std::unique_ptr<ASTBlock> ast_;
    SourcePosition position_;
    std::u32string name_;

    bool final_;
    bool overriding_;
    bool deprecated_;
    Mood mood_;
    bool unsafe_;
    bool forceInline_ = false;
    bool thunk_ = false;

    bool mutating_;
    bool external_ = false;
    bool closure_ = false;

    Function *virtualTableThunk_ = nullptr;

    std::string externalName_;
    AccessLevel access_;
    TypeDefinition *owner_;
    Package *package_;
    std::u32string documentation_;
    FunctionType functionType_;
    size_t variableCount_ = 0;
    MFFlowCategory memoryFlowTypeThis_;
};

}  // namespace EmojicodeCompiler

#endif /* Function_hpp */
