//
//  EmojicodeCompiler.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Emojicode_EmojicodeCompiler_h
#define Emojicode_EmojicodeCompiler_h

#define EmojicodeCompiler

#include "EmojicodeShared.h"
#include "Emojis.h"

#include <array>
#include <vector>
#include <map>
#include <string>

class Class;
class Method;
class Initializer;
class ClassMethod;
class Protocol;
class Enum;
struct Scope;
struct ScopeWrapper;
struct Token;

#include "Type.hpp"

class EmojicodeString: public std::basic_string<EmojicodeChar>  {
public:
    char* utf8CString() const;
};

struct StartingFlag {
    Class *eclass;
    ClassMethod *method;
};
extern StartingFlag startingFlag;
extern bool foundStartingFlag;

class Package {
public:
    Package(const char *n, PackageVersion v, bool r) : name(n), version(v), requiresNativeBinary(r) {}
    
    const char *name;
    PackageVersion version;
    bool requiresNativeBinary;
};

struct Variable {
    Variable(const Token *n, Type t) : name(n), type(t) {}
    
    /** The name of the variable */
    const Token *name;
    /** The type */
    Type type;
};

/*
 * MARK: Classes
 */

/** Class */
class Class {
public:
    Class() {}
    
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar enamespace;
    /** Whether this eclass eligible for initializer inheritance */
    bool inheritsContructors;
    
    /** The eclass' superclass. NULL if the eclass has no superclass. */
    Class *superclass = NULL;
    
    uint16_t index;
    
    /** The variable names. */
    std::vector<Variable *> instanceVariables;
    /** Deinitializer for primitive types */
    void (*deconstruct)(void *);
    
    /** List of all methods for user classes */
    std::vector<Method *> methodList;
    std::vector<Initializer *> initializerList;
    std::vector<ClassMethod *> classMethodList;
    std::vector<Initializer *> requiredInitializerList;
    std::vector<Protocol *> protocols;
    
    const Token *classBegin;
    const Token *documentationToken;
    
    uint16_t nextMethodVti;
    uint16_t nextClassMethodVti;
    uint16_t nextInitializerVti;
    
    /** The number of generic arguments including those from a super eclass. */
    uint16_t genericArgumentCount;
    /** The number of generic arguments this eclass takes. */
    uint16_t ownGenericArgumentCount;
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
    /** Gets a initializer by its name returns @c NULL if the eclass does not have an initializer with this name */
    Initializer* getInitializer(EmojicodeChar name);
    /** Returns a method of object by name */
    ClassMethod* getClassMethod(EmojicodeChar name);

    std::map<EmojicodeChar, Method*> methods;
    std::map<EmojicodeChar, ClassMethod*> classMethods;
    std::map<EmojicodeChar, Initializer*> initializers;
};

/** Fetch a class by its name and enamespace. Returns NULL if the class cannot be found. */
extern Class* getClass(EmojicodeChar name, EmojicodeChar enamespace);


//MARK: Protocols

class Enum {
public:
    Enum(EmojicodeChar name, Package& package, const Token *dt) : name(name), package(package), documentationToken(dt) {}
    
    EmojicodeChar name;
    std::map<EmojicodeChar, EmojicodeInteger> map;
    /** The package in which this eclass was defined. */
    Package& package;
    
    const Token *documentationToken;
    
    std::pair<bool, EmojicodeInteger> getValueFor(EmojicodeChar c) const;
    void addValueFor(EmojicodeChar c);
private:
    EmojicodeInteger valuesCounter = 0;
};

/** Returns the protocol with name @c name in enamespace @c namepsace or @c NULL if the protocol cannot be found. */
extern Enum* getEnum(EmojicodeChar name, EmojicodeChar enamespace);

//MARK: Protocol

class Protocol {
public:
    Protocol(EmojicodeChar n, EmojicodeChar ns, uint_fast16_t i) : name(n), enamespace(ns), index(i) {}
    
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar enamespace;
    
    /** List of all methods. */
    std::vector<Method *> methodList;
    
    /** Hashmap holding methods. @warning Don't access it directly, use the correct functions. */
    std::map<EmojicodeChar, Method *> methods;
    
    uint_fast16_t index;
    
    /** The package in which this eclass was defined. */
    Package *package;
    
    const Token *documentationToken;
    
    Method* getMethod(EmojicodeChar c);
};

/** Returns the protocol with name @c name in enamespace @c namepsace or @c NULL if the protocol cannot be found. */
extern Protocol* getProtocol(EmojicodeChar name, EmojicodeChar enamespace);

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_ENUMERATOR;
extern Protocol *PR_ENUMERATEABLE;

extern std::map<std::array<EmojicodeChar, 2>, Class*> classesRegister;
extern std::map<std::array<EmojicodeChar, 2>, Protocol*> protocolsRegister;
extern std::map<std::array<EmojicodeChar, 2>, Enum*> enumsRegister;

extern std::vector<Class *> classes;
extern std::vector<Package *> packages;


//MARK: Errors

/**
 * Issues a compiler error and exits the compiler.
 * @param token Used to determine the error location. If @c NULL the error origin is the beginning of the document.
 */
_Noreturn void compilerError(const Token *token, const char *err, ...);

/**
 * Issues a compiler warning. The compilation is continued afterwards.
 * @param token Used to determine the error location. If @c NULL the error origin is the beginning of the document.
 */
void compilerWarning(const Token *token, const char *err, ...);

/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);


//MARK: Lexer

extern const Token* currentToken;
extern const Token* consumeToken();
#define nextToken() (currentToken->nextToken)

void report(const char *packageName);

#endif
