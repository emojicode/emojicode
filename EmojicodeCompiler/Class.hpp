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
#include "Function.hpp"
#include "TypeContext.hpp"
#include "Variable.hpp"

class Type;

class Class : public TypeDefinitionFunctional {
public:
    static const std::list<Class *>& classes() { return classes_; }
    
    Class(EmojicodeChar name, Package *pkg, SourcePosition position, const EmojicodeString &documentation);
    
    /** Whether this class eligible for initializer inheritance. */
    bool inheritsContructors = false;
    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class *superclass = nullptr;
    
    uint16_t index;
    
    bool canBeUsedToResolve(TypeDefinitionFunctional *a);
    
    /** All instance variables. */
    const std::vector<Variable>& instanceVariables() { return instanceVariables_; }
    /** Adds an instance variable. */
    void addInstanceVariable(const Variable&);
    /** Returns a list of all required intializers. */
    const std::set<EmojicodeChar>& requiredInitializers() const { return requiredInitializers_; }
    
    uint16_t nextMethodVti;
    uint16_t nextInitializerVti;

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from);
    
    /** Declares that this class agrees to the given protocol. */
    bool addProtocol(Type type);
    /** Returns a list of all protocols to which this class conforms. */
    const std::list<Type>& protocols() const { return protocols_; };
    
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual Method* lookupMethod(EmojicodeChar name);
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    virtual Initializer* lookupInitializer(EmojicodeChar name);
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    virtual ClassMethod* lookupClassMethod(EmojicodeChar name);
private:
    static std::list<Class *> classes_;
    
    std::list<Type> protocols_;
    std::set<EmojicodeChar> requiredInitializers_;
    std::vector<Variable> instanceVariables_;
    
    virtual void handleRequiredInitializer(Initializer *init);
};

#endif /* Class_hpp */
