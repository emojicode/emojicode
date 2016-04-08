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

class TypeDefinition {
public:
    /** Returns a documentation token documenting this type definition or @c nullptr. */
    const Token* documentationToken() const { return documentationToken_; }
    /** Returns the name of the type definition. */
    EmojicodeChar name() const { return name_; }
    /** Returns the package in which this type was defined. */
    Package* package() const { return package_; }
protected:
    TypeDefinition(EmojicodeChar name, Package *p, const Token *dToken) : name_(name), package_(p), documentationToken_(dToken) {}
    const Token *documentationToken_;
    EmojicodeChar name_;
    Package *package_;
};

class Class : public TypeDefinition {
public:
    Class(EmojicodeChar name, const Token *classBegin, Package *pkg, const Token *dToken)
        : classBeginToken_(classBegin), TypeDefinition(name, pkg, dToken) {}
    
    /** Whether this eclass eligible for initializer inheritance. */
    bool inheritsContructors = false;
    
    /** The class's superclass. @c nullptr if the class has no superclass. */
    Class *superclass = nullptr;
    
    uint16_t index;
    
    const Token* classBeginToken() const { return classBeginToken_; }
    
    /** The variable names. */
    std::vector<Variable *> instanceVariables;
    
    /** List of all methods for user classes */
    std::vector<Method *> methodList;
    std::vector<Initializer *> initializerList;
    std::vector<ClassMethod *> classMethodList;
    std::vector<Initializer *> requiredInitializerList;
    
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
    
    /** Returns true if @c a or a superclass of @c a conforms to @c to. */
    bool conformsTo(Protocol *to);
    
    /** Returns true if @c a inherits from eclass @c from */
    bool inheritsFrom(Class *from);
    
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    Method* getMethod(const Token *token, Type type, TypeContext typeContext);
    /** Returns an initializer by the given identifier token or issues an error if the initializer does not exist. */
    Initializer* getInitializer(const Token *token, Type type, TypeContext typeContext);
    /** Returns a method by the given identifier token or issues an error if the method does not exist. */
    ClassMethod* getClassMethod(const Token *token, Type type, TypeContext typeContext);
    
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    Method* lookupMethod(EmojicodeChar name);
    /** Returns a initializer by the given identifier token or @c nullptr if the initializer does not exist. */
    Initializer* lookupInitializer(EmojicodeChar name);
    /** Returns a method by the given identifier token or @c nullptr if the method does not exist. */
    ClassMethod* lookupClassMethod(EmojicodeChar name);
    
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
    
    const Token *classBeginToken_;
};

class Protocol : public TypeDefinition {
public:
    Protocol(EmojicodeChar name, uint_fast16_t i, Package *pkg, const Token *dt)
        : index(i), TypeDefinition(name, pkg, dt) {}
    
    uint_fast16_t index;
    
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
    Enum(EmojicodeChar name, Package *package, const Token *dt)
        : TypeDefinition(name, package, dt) {}
    
    std::map<EmojicodeChar, EmojicodeInteger> map;
    
    std::pair<bool, EmojicodeInteger> getValueFor(EmojicodeChar c) const;
    void addValueFor(EmojicodeChar c);
private:
    EmojicodeInteger valuesCounter = 0;
};

#endif /* Class_hpp */
