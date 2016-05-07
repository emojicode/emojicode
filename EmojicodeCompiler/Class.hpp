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
#include "TypeDefinitionWithGenerics.hpp"
#include "Procedure.hpp"

struct TypeContext;
class Type;

class Class : public TypeDefinitionWithGenerics {
public:
    static const std::list<Class *>& classes() { return classes_; }
    
    Class(EmojicodeChar name, SourcePosition p, Package *pkg, const EmojicodeString &documentation);
    
    /** Whether this eclass eligible for initializer inheritance. */
    bool inheritsContructors = false;
    
    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class *superclass = nullptr;
    
    uint16_t index;
    
    const SourcePosition& position() const { return position_; }
    
    bool canBeUsedToResolve(TypeDefinitionWithGenerics *a);
    
    /** All instance variables. */
    const std::vector<Argument>& instanceVariables() { return instanceVariables_; }
    /** Adds an instance variable. */
    void addInstanceVariable(const Argument&);
    
    /** List of all methods for user classes */
    std::vector<Method *> methodList;
    std::vector<Initializer *> initializerList;
    std::vector<ClassMethod *> classMethodList;
    const std::set<EmojicodeChar>& requiredInitializers() const { return requiredInitializers_; }
    
    uint16_t nextMethodVti;
    uint16_t nextClassMethodVti;
    uint16_t nextInitializerVti;

    /** Returns true if @c a inherits from eclass @c from */
    bool inheritsFrom(Class *from);
    
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    Method* getMethod(const Token &token, Type type, TypeContext typeContext);
    /** Returns an initializer by the given identifier token or issues an error if the initializer does not exist. */
    Initializer* getInitializer(const Token &token, Type type, TypeContext typeContext);
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    ClassMethod* getClassMethod(const Token &token, Type type, TypeContext typeContext);
    
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    Method* lookupMethod(EmojicodeChar name);
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    Initializer* lookupInitializer(EmojicodeChar name);
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    ClassMethod* lookupClassMethod(EmojicodeChar name);
    
    void addMethod(Method *method);
    void addInitializer(Initializer *method);
    void addClassMethod(ClassMethod *method);
    
    /** Declares that this class agrees to the given protocol. */
    bool addProtocol(Type type);
    /** Returns a list of all protocols to which this class conforms. */
    const std::list<Type>& protocols() const { return protocols_; };
private:
    static std::list<Class *> classes_;
    
    std::map<EmojicodeChar, Method *> methods_;
    std::map<EmojicodeChar, ClassMethod *> classMethods_;
    std::map<EmojicodeChar, Initializer *> initializers_;
    
    std::list<Type> protocols_;
    std::set<EmojicodeChar> requiredInitializers_;
    std::vector<Argument> instanceVariables_;
    
    SourcePosition position_;
};

#endif /* Class_hpp */
