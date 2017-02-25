//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include <list>
#include <vector>
#include <map>
#include <set>
#include "Package.hpp"
#include "TypeDefinitionFunctional.hpp"
#include "TypeContext.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "VTIProvider.hpp"

class Type;

class Class : public TypeDefinitionFunctional {
public:
    static const std::vector<Class *>& classes() { return classes_; }

    Class(EmojicodeString name, Package *pkg, SourcePosition p, const EmojicodeString &documentation, bool final);

    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class* superclass() const { return superclass_; }
    /** Sets the class superclass to the given class. */
    void setSuperclass(Class *);

    uint16_t index;

    bool canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) const override;

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from) const;

    /** Whether this class can be subclassed. */
    bool final() const { return final_; }
    /** Whether this class is eligible for initializer inheritance. */
    bool inheritsInitializers() const { return inheritsInitializers_; }
    /** Returns a list of all required intializers. */
    const std::set<EmojicodeString>& requiredInitializers() const { return requiredInitializers_; }

    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    Function* lookupMethod(const EmojicodeString &name) override;
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    Initializer* lookupInitializer(const EmojicodeString &name) override;
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    Function* lookupTypeMethod(const EmojicodeString &name) override;

    void finalize() override;

    /** Returns the number of initializers including those inherited from the superclass.
     @warning @c finalize() must be called before a call to this method. */
    size_t fullInitializerCount() const { return initializerVtiProvider_.peekNext(); }
    /** Returns the number of methods and type methods including those inherited from the superclass.
     @warning @c finalize() must be called before a call to this method. */
    size_t fullMethodCount() const { return methodVtiProvider_.peekNext(); }

    int usedMethodCount() { return methodVtiProvider_.usedCount(); }
    int usedInitializerCount() { return initializerVtiProvider_.usedCount(); }
private:
    static std::vector<Class *> classes_;

    std::set<EmojicodeString> requiredInitializers_;

    bool final_;
    bool inheritsInitializers_ = false;

    ClassVTIProvider methodVtiProvider_;
    ClassVTIProvider initializerVtiProvider_;

    Class *superclass_ = nullptr;

    void handleRequiredInitializer(Initializer *init) override;
};

#endif /* Class_hpp */
