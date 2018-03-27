//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

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

    /// Makes hasSubclass() return true.
    void setHasSubclass() { hasSubclass_ = true; }
    /// @returns true if this class has a subclass.
    /// @see setHasSubclass()
    bool hasSubclass() const { return hasSubclass_; }

    void setFinal() { final_ = true; }
private:
    std::set<std::u32string> requiredInitializers_;

    /// @pre superclass() != nullptr
    Function* findSuperFunction(Function *function) const;
    /// @pre superclass() != nullptr
    Initializer* findSuperInitializer(Initializer *function) const;

    /// Checks that @c function, if at all, is a valid override. If it is, function is assigned the super functions
    /// virtual table index.
    /// @pre superclass() != nullptr
    /// @throws CompilerError if the override is improper, e.g. implicit
    void checkOverride(Function *function, SemanticAnalyser *analyser);
    /// Checks whether initializer is the implementation of a required initializer. If it is the method validates
    /// that the implementation is valid and assigns it the super initializers virtual table index.
    /// @pre superclass() != nullptr
    void checkInheritedRequiredInit(Initializer *initializer);

    std::vector<llvm::Constant *> virtualTable_;

    bool final_;
    bool foreign_;
    bool inheritsInitializers_ = false;
    bool hasSubclass_ = false;

    llvm::GlobalVariable *classMeta_ = nullptr;

    void handleRequiredInitializer(Initializer *init) override;
};

}  // namespace EmojicodeCompiler

#endif /* Class_hpp */
