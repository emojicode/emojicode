//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 09/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

class Class {
public:
    Class() {}
    
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar enamespace;
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
    std::vector<Type> genericArgumentContraints;
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
    
    const std::vector<Protocol*>& protocols() { return protocols_; };
private:
    std::map<EmojicodeChar, Method *> methods;
    std::map<EmojicodeChar, ClassMethod *> classMethods;
    std::map<EmojicodeChar, Initializer *> initializers;
    
    std::vector<Protocol *> protocols_;
};

/** Fetch a class by its name and enamespace. Returns nullptr if the class cannot be found. */
extern Class* getClass(EmojicodeChar name, EmojicodeChar enamespace);


class Protocol {
public:
    Protocol(EmojicodeChar n, EmojicodeChar ns, uint_fast16_t i) : name(n), enamespace(ns), index(i) {}
    
    /** The name of the protocol. */
    EmojicodeChar name;
    /** The namespace to which this protocol belongs. */
    EmojicodeChar enamespace;
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

/** Returns the protocol with name @c name in enamespace @c namepsace or @c nullptr if the protocol cannot be found. */
extern Protocol* getProtocol(EmojicodeChar name, EmojicodeChar enamespace);

#endif /* Class_hpp */
