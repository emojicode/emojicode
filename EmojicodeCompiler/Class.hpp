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
    static const std::list<Class *>& classes() { return classes_; }
    
    Class(EmojicodeChar name, Package *pkg, SourcePosition position, const EmojicodeString &documentation, bool final);
    
    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class* superclass() const { return superclass_; };
    /** Sets the class superclass to the given class. */
    void setSuperclass(Class *);
    
    uint16_t index;
    
    bool canBeUsedToResolve(TypeDefinitionFunctional *a);
    
    /** All instance variables. */
    const std::vector<Variable>& instanceVariables() { return instanceVariables_; }
    /** Adds an instance variable. */
    void addInstanceVariable(const Variable&);
    /** Returns a list of all required intializers. */
    const std::set<EmojicodeChar>& requiredInitializers() const { return requiredInitializers_; }

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from) const;
    
    /** Whether this class can be subclassed. */
    bool final() const { return final_; }
    /** Whether this class is eligible for initializer inheritance. */
    bool inheritsInitializers() const { return inheritsInitializers_; }
    
    /** Declares that this class agrees to the given protocol. */
    void addProtocol(Type type);
    /** Returns a list of all protocols to which this class conforms. */
    const std::list<Type>& protocols() const { return protocols_; };
    
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function* lookupMethod(EmojicodeChar name);
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    virtual Initializer* lookupInitializer(EmojicodeChar name);
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Function* lookupClassMethod(EmojicodeChar name);
    
    virtual void finalize();
    
    /** Returns an object scope for an instance of the defined type.
        @warning @c finalize() must be called before a call to this method. */
    Scope& objectScope() { return objectScope_; }
    /** Returns the number of instance variables including those inherited from the superclass.
        @warning @c finalize() must be called before a call to this method. */
    size_t fullInstanceVariableCount() { return nextInstanceVariableID_; }
    /** Returns the number of initializers including those inherited from the superclass.
        @warning @c finalize() must be called before a call to this method. */
    size_t fullInitializerCount() { return initializerVtiProvider_.peekNext(); }
    /** Returns the number of methods and type methods including those inherited from the superclass.
        @warning @c finalize() must be called before a call to this method. */
    size_t fullMethodCount() { return methodVtiProvider_.peekNext(); }

    int usedMethodCount() { return methodVtiProvider_.usedCount(); }
    int usedInitializerCount() { return initializerVtiProvider_.usedCount(); }
private:
    static std::list<Class *> classes_;
    
    std::list<Type> protocols_;
    std::set<EmojicodeChar> requiredInitializers_;
    std::vector<Variable> instanceVariables_;
    
    bool final_;
    bool inheritsInitializers_;
    
    ClassVTIProvider methodVtiProvider_;
    ClassVTIProvider initializerVtiProvider_;

    Scope objectScope_;
    Class *superclass_ = nullptr;
    
    size_t nextInstanceVariableID_ = 0;
    
    virtual void handleRequiredInitializer(Initializer *init);
};

#endif /* Class_hpp */
