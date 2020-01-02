//
//  TypeDefinition.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef TypeDefinition_hpp
#define TypeDefinition_hpp

#include "CompilerError.hpp"
#include "Generic.hpp"
#include "Package/Definition.hpp"
#include "Type.hpp"
#include "Functions/FunctionResolver.hpp"
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace llvm {
class Constant;
class Function;
class GlobalVariable;
class Type;
}  // namespace llvm

namespace EmojicodeCompiler {

class ASTExpr;
class Scope;
enum class Mood;

struct InstanceVariableDeclaration {
    InstanceVariableDeclaration() = delete;
    InstanceVariableDeclaration(std::u32string name, std::unique_ptr<ASTType> type, SourcePosition pos)
    : name(std::move(name)), type(std::move(type)), position(std::move(pos)) {}
    std::u32string name;
    std::shared_ptr<ASTType> type;
    SourcePosition position;
    std::shared_ptr<ASTExpr> expr;
};

struct ProtocolConformance {
    explicit ProtocolConformance(std::shared_ptr<ASTType> &&type) : type(std::move(type)) {}

    std::shared_ptr<ASTType> type;
    /// Contains the functions that will be placed in the dispatch table in same order as the method list of the protocol.
    std::vector<Function*> implementations;
};

struct TypeDefinitionReification {
    llvm::Type *type = nullptr;
};

class TypeDefinition : public Generic<TypeDefinition, TypeDefinitionReification>, public Definition {
public:
    TypeDefinition(const TypeDefinition&) = delete;

    virtual Type type() = 0;

    /// The generic arguments of the super type.
    /// @returns The generic arguments of the Type passed to setSuperType().
    /// If no super type was provided an empty vector is returned.
    virtual std::vector<Type> superGenericArguments() const { return std::vector<Type>(); }

    /// Determines whether the resolution constraint of TypeType::GenericVariable allows it to be resolved on an Type
    /// instance representing an instance of this TypeDefinition.
    /// @see Type::resolveOn
    virtual bool canResolve(TypeDefinition *resolutionConstraint) const = 0;

    /// Determines whether an instance of this type can be created by the literal described by `literal`.
    /// This method shall not take generic arguments into account. This is handled by Type::compatibleTo.
    virtual bool canInitFrom(const Type &literal) const { return false; }

    virtual void addInstanceVariable(const InstanceVariableDeclaration&);

    const FunctionResolver<Function>& methods() const { return methods_; }
    const FunctionResolver<Function>& typeMethods() const { return typeMethods_; }
    const FunctionResolver<Initializer>& inits() const { return inits_; }
    FunctionResolver<Function>& methods() { return methods_; }
    FunctionResolver<Function>& typeMethods() { return typeMethods_; }
    FunctionResolver<Initializer>& inits() { return inits_; }

    /// Declares conformance of the type to a protocol.
    void addProtocol(std::shared_ptr<ASTType> type) { protocols_.emplace_back(std::move(type)); }
    /** Returns a list of all protocols to which this class conforms. */
    std::vector<ProtocolConformance>& protocols() { return protocols_; }

    /// Calls the given function with every Function that is defined for this TypeDefinition, i.e. all methods,
    /// type methods and initializers.
    void eachFunction(const std::function<void(Function *)>& cb) const;
    /// Calls the given function with every function but no that is defined within this TypeDefinition, i.e. all methods,
    /// type methods and initializers.
    void eachFunctionWithoutInitializers(const std::function<void(Function *)>& cb) const;

    /// @retunrs A reference to the instance scope for SemanticAnalysis.
    Scope& instanceScope() { return *scope_; }

    /// Whether the type was exported in the package it was defined in.
    bool exported() const { return exported_; }

    /// The destructor is responsible for releasing all instance variables and releasing or freeing the generic
    /// arguments if applicable. It is used for value types and classes.
    /// In classes, the destructor calls the deinitializer of the class and all superclasses.
    /// @see Class::deinitializer().
    llvm::Function* destructor() { return destructor_; }
    void setDestructor(llvm::Function *fn) { destructor_ = fn; }

    virtual bool storesGenericArgs() const { return false; }
    
    bool isGenericDynamismDisabled() const { return genericDynamismDisabled_; }
    void disableGenericDynamism() { genericDynamismDisabled_ = true; }

    const std::pair<llvm::Function*, llvm::Function*>& boxRetainRelease() const { return boxRetainRelease_; }
    void setBoxRetainRelease(std::pair<llvm::Function*, llvm::Function*> pair) { boxRetainRelease_ = pair; }

    /// Contains the type’s instance variables. In a class, this will also contain inherited instance variables after
    /// calling Class::inherit().
    const std::vector<InstanceVariableDeclaration>& instanceVariables() const { return instanceVariables_; }
    std::vector<InstanceVariableDeclaration>& instanceVariablesMut() { return instanceVariables_; }

    void setProtocolTables(std::map<Type, llvm::Constant*> &&tables) { protocolTables_ = std::move(tables); }
    llvm::Constant* protocolTableFor(const Type &type) { return protocolTables_.find(type)->second; }
    const std::map<Type, llvm::Constant*>& protocolTables() { return protocolTables_; }

protected:
    TypeDefinition(std::u32string name, Package *p, SourcePosition pos, std::u32string documentation, bool exported);

    std::vector<ProtocolConformance> protocols_;

    virtual ~TypeDefinition();

private:
    std::unique_ptr<Scope> scope_;

    FunctionResolver<Function> methods_;
    FunctionResolver<Function> typeMethods_;
    FunctionResolver<Initializer> inits_;

    std::pair<llvm::Function*, llvm::Function*> boxRetainRelease_;

    bool exported_;
    bool genericDynamismDisabled_ = false;

    std::map<Type, llvm::Constant*> protocolTables_;

    std::vector<InstanceVariableDeclaration> instanceVariables_;

    llvm::Function *destructor_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* TypeDefinition_hpp */
