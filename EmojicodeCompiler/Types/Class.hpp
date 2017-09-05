//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "../Generation/VTIProvider.hpp"
#include "../Package/Package.hpp"
#include "../Scoping/Scope.hpp"
#include "../Scoping/Variable.hpp"
#include "../Types/TypeContext.hpp"
#include "TypeDefinition.hpp"
#include <list>
#include <map>
#include <set>
#include <vector>

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

    uint16_t index;

    bool canBeUsedToResolve(TypeDefinition *resolutionConstraint) const override;

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from) const;

    /** Whether this class can be subclassed. */
    bool final() const { return final_; }
    /** Whether this class is eligible for initializer inheritance. */
    bool inheritsInitializers() const { return inheritsInitializers_; }
    /** Returns a list of all required intializers. */
    const std::set<std::u32string>& requiredInitializers() const { return requiredInitializers_; }

    Function* lookupMethod(const std::u32string &name) override;
    Initializer* lookupInitializer(const std::u32string &name) override;
    Function* lookupTypeMethod(const std::u32string &name) override;

    void prepareForCG() override;
    void prepareForSemanticAnalysis() override;

    /** Returns the number of initializers including those inherited from the superclass.
     @warning @c prepareForCG() must be called before a call to this method. */
    size_t fullInitializerCount() const { return initializerVtiProvider_.peekNext(); }
    /** Returns the number of methods and type methods including those inherited from the superclass.
     @warning @c prepareForCG() must be called before a call to this method. */
    size_t fullMethodCount() const { return methodVtiProvider_.peekNext(); }

    int usedMethodCount() { return methodVtiProvider_.usedCount(); }
    int usedInitializerCount() { return initializerVtiProvider_.usedCount(); }
private:
    VTIProvider *protocolMethodVtiProvider() override {
        return &methodVtiProvider_;
    }
    std::set<std::u32string> requiredInitializers_;

    Function* findSuperFunction(Function *function) const;
    Initializer* findSuperFunction(Initializer *function) const;
    /// Checks that @c function, if at all, is a valid override.
    /// @throws CompilerError if the override is improper, e.g. implicit
    void checkOverride(Function *function);
    void overrideFunction(Function *function);

    bool final_;
    bool inheritsInitializers_ = false;

    ClassVTIProvider methodVtiProvider_;
    ClassVTIProvider initializerVtiProvider_;

    void handleRequiredInitializer(Initializer *init) override;
};

}  // namespace EmojicodeCompiler

#endif /* Class_hpp */
