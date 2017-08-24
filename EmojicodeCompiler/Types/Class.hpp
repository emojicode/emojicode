//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "../FunctionType.hpp"
#include "../Generation/VTIProvider.hpp"
#include "../Parsing/Package.hpp"
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
    static const std::vector<Class *>& classes() { return classes_; }

    Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool final);

    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class* superclass() const { return superclass_; }
    /** Sets the class superclass to the given class. */
    void setSuperclass(Class *);

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

    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    Function* lookupMethod(const std::u32string &name) override;
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    Initializer* lookupInitializer(const std::u32string &name) override;
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
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
    void createCGScope() override;

    static std::vector<Class *> classes_;

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

    Class *superclass_ = nullptr;

    void handleRequiredInitializer(Initializer *init) override;
};

}  // namespace EmojicodeCompiler

#endif /* Class_hpp */
