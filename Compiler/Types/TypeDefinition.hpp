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
#include "Lex/SourcePosition.hpp"
#include "Scoping/Scope.hpp"
#include "Functions/Mood.hpp"
#include "Type.hpp"
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

struct InstanceVariableDeclaration {
    InstanceVariableDeclaration() = delete;
    InstanceVariableDeclaration(std::u32string name, std::unique_ptr<ASTType> type, SourcePosition pos)
    : name(std::move(name)), type(std::move(type)), position(std::move(pos)) {}
    std::u32string name;
    std::shared_ptr<ASTType> type;
    SourcePosition position;
    std::shared_ptr<ASTExpr> expr;
};

struct TypeDefinitionReification {
    llvm::Type *type = nullptr;
};

class TypeDefinition : public Generic<TypeDefinition, TypeDefinitionReification> {
public:
    TypeDefinition(const TypeDefinition&) = delete;

    virtual Type type() = 0;

    /// Returns a documentation token documenting this type definition.
    const std::u32string& documentation() const { return documentation_; }
    /// Returns the name of the type definition.
    std::u32string name() const { return name_; }
    /// Returns the package in which this type was defined.
    Package* package() const { return package_; }
    /// The position at which this type was initially defined.
    const SourcePosition& position() const { return position_; }

    /// The generic arguments of the super type.
    /// @returns The generic arguments of the Type passed to setSuperType().
    /// If no super type was provided an empty vector is returned.
    virtual std::vector<Type> superGenericArguments() const { return std::vector<Type>(); }

    /// Determines whether the resolution constraint of TypeType::GenericVariable allows it to be resolved on an Type
    /// instance representing an instance of this TypeDefinition.
    /// @see Type::resolveOn
    virtual bool canResolve(TypeDefinition *resolutionConstraint) const = 0;

    /// Returns a method by the given identifier token or throws an exception if the method does not exist.
    /// @throws CompilerError
    Function *getMethod(const std::u32string &name, const Type &type, const TypeContext &typeContext, Mood mood,
                            const SourcePosition &p) const;
    /// Returns an initializer by the given identifier token or throws an exception if the method does not exist.
    /// @throws CompilerError
    Initializer* getInitializer(const std::u32string &name, const Type &type, const TypeContext &typeContext,
                                const SourcePosition &p) const;
    /// Returns a method by the given identifier token or throws an exception if the method does not exist.
    /// @throws CompilerError
    Function *getTypeMethod(const std::u32string &name, const Type &type, const TypeContext &typeContext,
                            Mood mood, const SourcePosition &p) const;

    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function *lookupMethod(const std::u32string &name, Mood mood) const;
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    virtual Initializer* lookupInitializer(const std::u32string &name) const;
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function *lookupTypeMethod(const std::u32string &name, Mood mood) const;

    Function* addMethod(std::unique_ptr<Function> &&method);
    Initializer* addInitializer(std::unique_ptr<Initializer> &&initializer);
    Function* addTypeMethod(std::unique_ptr<Function> &&method);
    virtual void addInstanceVariable(const InstanceVariableDeclaration&);

    /// A vector containing all methods in the order they were added via ::addMethod
    const std::vector<Function *>& methodList() const { return methodList_; }
    /// A vector containing all initializers in the order they were added via ::addInitializer
    const std::vector<Initializer *>& initializerList() const { return initializerList_; }
    /// A vector containing all type methods in the order they were added via ::addTypeMethod
    const std::vector<Function *>& typeMethodList() const { return typeMethodList_; }

    /** Declares that this class agrees to the given protocol. */
    void addProtocol(std::shared_ptr<ASTType> type) { protocols_.emplace_back(std::move(type)); }
    /** Returns a list of all protocols to which this class conforms. */
    const std::vector<std::shared_ptr<ASTType>>& protocols() const { return protocols_; };

    /// Calls the given function with every Function that is defined for this TypeDefinition, i.e. all methods,
    /// type methods and initializers.
    void eachFunction(const std::function<void(Function *)>& cb) const;
    /// Calls the given function with every function but no that is defined within this TypeDefinition, i.e. all methods,
    /// type methods and initializers.
    void eachFunctionWithoutInitializers(const std::function<void(Function *)>& cb) const;

    /// @retunrs A reference to the instance scope for SemanticAnalysis.
    Scope& instanceScope() { return scope_; }

    /// Whether the type was exported in the package it was defined.
    bool exported() const { return exported_; }

    Function* deinitializer();

    const std::pair<llvm::Function*, llvm::Function*>& boxRetainRelease() const { return boxRetainRelease_; }
    void setBoxRetainRelease(std::pair<llvm::Function*, llvm::Function*> pair) { boxRetainRelease_ = pair; }

    const std::vector<InstanceVariableDeclaration>& instanceVariables() const { return instanceVariables_; }

    void setProtocolTables(std::map<Type, llvm::Constant*> &&tables) { protocolTables_ = std::move(tables); }
    llvm::Constant* protocolTableFor(const Type &type) { return protocolTables_.find(type)->second; }
    const std::map<Type, llvm::Constant*>& protocolTables() { return protocolTables_; }

protected:
    TypeDefinition(std::u32string name, Package *p, SourcePosition pos, std::u32string documentation, bool exported);

    std::vector<std::shared_ptr<ASTType>> protocols_;

    std::vector<InstanceVariableDeclaration>& instanceVariablesMut() { return instanceVariables_; }

    /// Called if a required initializer is passed to addInitializer().
    /// @param init The initializer passed to addInitializer().
    virtual void handleRequiredInitializer(Initializer *init);

    template <typename T>
    void duplicateDeclarationCheck(T *function, const std::map<std::u32string, std::unique_ptr<T>> &dict) {
        if (dict.find(methodTableName(function->name(), function->mood())) != dict.end()) {
            throw CompilerError(function->position(), utf8(function->name()), " is declared twice.");
        }
    }

    virtual ~TypeDefinition();

private:
    Scope scope_;

    std::map<std::u32string, std::unique_ptr<Function>> methods_;
    std::map<std::u32string, std::unique_ptr<Function>> typeMethods_;
    std::map<std::u32string, std::unique_ptr<Initializer>> initializers_;

    std::vector<Function *> methodList_;
    std::vector<Initializer *> initializerList_;
    std::vector<Function *> typeMethodList_;

    std::unique_ptr<Function> deinitializer_;

    std::pair<llvm::Function*, llvm::Function*> boxRetainRelease_;

    std::u32string name_;
    Package *package_;
    std::u32string documentation_;
    SourcePosition position_;
    bool exported_;

    std::map<Type, llvm::Constant*> protocolTables_;

    std::vector<InstanceVariableDeclaration> instanceVariables_;

    std::u32string methodTableName(const std::u32string &name, Mood mood) const;
};

}  // namespace EmojicodeCompiler

#endif /* TypeDefinition_hpp */
