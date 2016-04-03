//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "Package.hpp"

class Class : public TypeDefinition {
public:
    Class(EmojicodeChar name, const Token *classBegin, Package *pkg, const Token *dToken)
        : name(name), classBegin(classBegin), package(pkg), documentationToken(dToken) {}
    
    /** Self explaining */
    EmojicodeChar name;
    /** Whether this eclass eligible for initializer inheritance. */
    bool inheritsContructors = false;
    
    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class *superclass = nullptr;
    
    uint16_t index;
    
    /** The variable names. */
    std::vector<Variable *> instanceVariables;
    
    /** List of all methods for user classes */
    std::vector<Method *> methodList;
    std::vector<Initializer *> initializerList;
    std::vector<ClassMethod *> classMethodList;
    std::vector<Initializer *> requiredInitializerList;
    
    const Token *classBegin;
    const Token *documentationToken;
    
    uint16_t nextMethodVti;
    uint16_t nextClassMethodVti;
    uint16_t nextInitializerVti;
    
    /** The number of generic arguments including those from a super eclass. */
    uint16_t genericArgumentCount = 0;
    /** The number of generic arguments this eclass takes. */
    uint16_t ownGenericArgumentCount = 0;
    /** The types for the generic arguments. */
    std::vector<Type> genericArgumentConstraints;
    /** The arguments for the classes from which this eclass inherits. */
    std::vector<Type> superGenericArguments;
    /** Generic type arguments as variables */
    std::map<EmojicodeString, Type> ownGenericArgumentVariables;
    
    /** The package in which this eclass was defined. */
    Package *package;
    
    /** Returns true if @c a or a superclass of @c a conforms to @c to. */
    bool conformsTo(Protocol *to);
    
    /** Returns true if @c a inherits from eclass @c from */
    bool inheritsFrom(Class *from);
    
    /** Returns a method of object by name */
    Method* getMethod(EmojicodeChar name);
    /** Gets a initializer by its name returns @c nullptr if the eclass does not have an initializer with this name */
    Initializer* getInitializer(EmojicodeChar name);
    /** Returns a method of object by name */
    ClassMethod* getClassMethod(EmojicodeChar name);
    
    void addMethod(Method *method);
    void addInitializer(Initializer *method);
    void addClassMethod(ClassMethod *method);
    
    void addProtocol(Protocol *protocol);
    
    const std::vector<Protocol*>& protocols() const { return protocols_; };
private:
    std::map<EmojicodeChar, Method *> methods;
    std::map<EmojicodeChar, ClassMethod *> classMethods;
    std::map<EmojicodeChar, Initializer *> initializers;
    
    std::vector<Protocol *> protocols_;
};

class Protocol : public TypeDefinition {
public:
    Protocol(EmojicodeChar n, uint_fast16_t i, Package *pkg) : name(n), package(pkg), index(i) {}
    
    /** The name of the protocol. */
    EmojicodeChar name;
    /** The package in which this protocol was defined. */
    Package *package;
    
    uint_fast16_t index;
    
    const Token *documentationToken;
    
    Method* getMethod(EmojicodeChar c);
    void addMethod(Method *method);
    const std::vector<Method*>& methods() { return methodList_; };
private:
    /** List of all methods. */
    std::vector<Method *> methodList_;
    
    /** Hashmap holding methods. @warning Don't access it directly, use the correct functions. */
    std::map<EmojicodeChar, Method*> methods_;
};

class Enum : public TypeDefinition {
public:
    Enum(EmojicodeChar name, Package *package, const Token *dt) : name(name), package(package), documentationToken(dt) {}
    
    EmojicodeChar name;
    std::map<EmojicodeChar, EmojicodeInteger> map;
    /** The package in which this eclass was defined. */
    Package *package;
    
    const Token *documentationToken;
    
    std::pair<bool, EmojicodeInteger> getValueFor(EmojicodeChar c) const;
    void addValueFor(EmojicodeChar c);
private:
    EmojicodeInteger valuesCounter = 0;
};

#endif /* Class_hpp */
