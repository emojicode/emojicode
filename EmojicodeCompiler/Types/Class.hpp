//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "../Package/Package.hpp"
#include "../Types/TypeContext.hpp"
#include "TypeDefinition.hpp"
#include <set>

namespace llvm {
class GlobalVariable;
class Constant;
}  // namespace llvm

namespace EmojicodeCompiler {

class Type;

class Class : public TypeDefinition {
public:
    Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool final);

    /// The class's superclass.
    /// @returns TypeDefinition::superType().eclass(). Guaranteed to be @c nullptr if the class has no superclass.
    Class* superclass() const {
        if (superType().type() == TypeType::NoReturn) {
            return nullptr;
        }
        return superType().eclass();
    }

    using TypeDefinition::superType;

    /// @returns True iff this class inherits from @c from
    bool inheritsFrom(Class *from) const;
    /** Whether this class can be subclassed. */
    bool final() const { return final_; }
    /** Whether this class is eligible for initializer inheritance. */
    bool inheritsInitializers() const { return inheritsInitializers_; }
    /** Returns a list of all required intializers. */
    const std::set<std::u32string>& requiredInitializers() const { return requiredInitializers_; }

    void setClassInfo(llvm::GlobalVariable *classInfo) { classInfo_ = classInfo; }
    llvm::GlobalVariable* classInfo() { return classInfo_; }

    unsigned int virtualTableIndicesCount() { return virtualTableIndex_; }

    std::vector<llvm::Constant *>& virtualTable() { return virtualTable_; }

    Function* lookupMethod(const std::u32string &name) override;
    Initializer* lookupInitializer(const std::u32string &name) override;
    Function* lookupTypeMethod(const std::u32string &name) override;

    void prepareForSemanticAnalysis() override;
    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override;
private:
    std::set<std::u32string> requiredInitializers_;

    /// @pre superclass() != nullptr
    Function* findSuperFunction(Function *function) const;
    /// @pre superclass() != nullptr
    Initializer* findSuperFunction(Initializer *function) const;

    /// Checks that @c function, if at all, is a valid override. If it is, function is assigned the super functions
    /// virtual table index.
    /// @pre superclass() != nullptr
    /// @throws CompilerError if the override is improper, e.g. implicit
    void checkOverride(Function *function);
    /// Checks whether initializer is the implementation of a required initializer. If it is the method validates
    /// that the implementation is valid and assigns it the super initializers virtual table index.
    /// @pre superclass() != nullptr
    void checkInheritedRequiredInit(Initializer *initializer);

    unsigned int virtualTableIndex_ = 0;
    std::vector<llvm::Constant *> virtualTable_;

    bool final_;
    bool inheritsInitializers_ = false;

    llvm::GlobalVariable *classInfo_ = nullptr;

    void handleRequiredInitializer(Initializer *init) override;
};

}  // namespace EmojicodeCompiler

#endif /* Class_hpp */
