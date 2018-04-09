//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "Functions/Initializer.hpp"
#include "TypeDefinition.hpp"
#include "Types/TypeContext.hpp"
#include <set>

namespace llvm {
class GlobalVariable;
class Constant;
}  // namespace llvm

namespace EmojicodeCompiler {

class Type;
class SemanticAnalyser;

template <typename FT>
FT* ifNotPrivate(FT *function) {
    if (function == nullptr) {
        return nullptr;
    }
    return function->accessLevel() == AccessLevel::Private ? nullptr : function;
}

class Class : public TypeDefinition {
public:
    Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool exported,
          bool final, bool foreign);

    /// The class's superclass.
    /// @returns TypeDefinition::superType().eclass(). Guaranteed to be @c nullptr if the class has no superclass.
    Class* superclass() const {
        if (superType().type() == TypeType::NoReturn) {
            return nullptr;
        }
        return superType().klass();
    }

    using TypeDefinition::superType;

    /// @returns True iff this class inherits from @c from
    bool inheritsFrom(Class *from) const;
    /** Whether this class can be subclassed. */
    bool final() const { return final_; }
    /** Whether this class is eligible for initializer inheritance. */
    bool inheritsInitializers() const { return inheritsInitializers_; }

    bool foreign() const { return foreign_; }
    /** Returns a list of all required intializers. */
    const std::set<std::u32string>& requiredInitializers() const { return requiredInitializers_; }

    void setClassMeta(llvm::GlobalVariable *classInfo) { classMeta_ = classInfo; }
    llvm::GlobalVariable* classMeta() { return classMeta_; }

    std::vector<llvm::Constant *>& virtualTable() { return virtualTable_; }

    Function *lookupMethod(const std::u32string &name, bool imperative) const override;
    Initializer* lookupInitializer(const std::u32string &name) const override;
    Function *lookupTypeMethod(const std::u32string &name, bool imperative) const override;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override;
    void addInstanceVariable(const InstanceVariableDeclaration &declaration) override;

    void inherit(SemanticAnalyser *analyser);

    /// @pre superclass() != nullptr
    template <typename FT>
    FT* findSuperFunction(FT *function) const {
        switch (function->functionType()) {
            case FunctionType::ObjectMethod:
            case FunctionType::BoxingLayer:
                return ifNotPrivate(superclass()->lookupMethod(function->name(), function->isImperative()));
            case FunctionType::ClassMethod:
                return ifNotPrivate(superclass()->lookupTypeMethod(function->name(), function->isImperative()));
            case FunctionType::ObjectInitializer:
                return findSuperFunction(static_cast<Initializer *>(function));
            default:
                throw std::logic_error("Function of unexpected type in class");
        }
    }

    /// Makes hasSubclass() return true.
    void setHasSubclass() { hasSubclass_ = true; }
    /// @returns true if this class has a subclass.
    /// @see setHasSubclass()
    bool hasSubclass() const { return hasSubclass_; }

    void setFinal() { final_ = true; }

    void setVirtualFunctionCount(size_t n) { virtualFunctionCount_ = n; }
    size_t virtualFunctionCount() { return virtualFunctionCount_; }
private:
    std::set<std::u32string> requiredInitializers_;

    /// Checks that @c function, if at all, is a valid override. If it is, function is assigned the super functions
    /// virtual table index.
    /// @pre superclass() != nullptr
    /// @throws CompilerError if the override is improper, e.g. implicit
    void checkOverride(Function *function, SemanticAnalyser *analyser);
    /// Checks whether initializer is the implementation of a required initializer. If it is the method validates
    /// that the implementation is valid and assigns it the super initializers virtual table index.
    /// @pre superclass() != nullptr
    void checkInheritedRequiredInit(Initializer *initializer, SemanticAnalyser *analyser);

    std::vector<llvm::Constant *> virtualTable_;
    size_t virtualFunctionCount_ = 0;

    bool final_;
    bool foreign_;
    bool inheritsInitializers_ = false;
    bool hasSubclass_ = false;

    llvm::GlobalVariable *classMeta_ = nullptr;

    void handleRequiredInitializer(Initializer *init) override;
};

inline Initializer* ifRequired(Initializer *init) {
    if (init == nullptr) {
        return nullptr;
    }
    return init->required() ? init : nullptr;
}

template <>
inline Initializer* Class::findSuperFunction<Initializer>(Initializer *function) const {
    return ifRequired(ifNotPrivate(superclass()->lookupInitializer(function->name())));
}

}  // namespace EmojicodeCompiler

#endif /* Class_hpp */
