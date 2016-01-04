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
typedef struct Scope Scope;
typedef struct ScopeWrapper ScopeWrapper;
typedef struct Token Token;

class EmojicodeString: public std::basic_string<EmojicodeChar>  {
public:
    char* utf8CString() const;
};

std::vector<Class *> classes;

#include "Type.h"


typedef struct {
    Class *eclass;
    ClassMethod *method;
} StartingFlag;
StartingFlag startingFlag;
bool foundStartingFlag;

class Package {
public:
    Package(const char *n, PackageVersion v, bool r) : name(n), version(v), requiresNativeBinary(r) {}
    
    const char *name;
    PackageVersion version;
    bool requiresNativeBinary;
};

std::vector<Package *> packages;

typedef enum {
    PUBLIC, PRIVATE, PROTECTED
} AccessLevel;



//MARK: Tokens

enum TokenType {
    NO_TYPE,
    STRING,
    COMMENT,
    DOCUMENTATION_COMMENT,
    INTEGER,
    DOUBLE,
    BOOLEAN_TRUE,
    BOOLEAN_FALSE,
    IDENTIFIER,
    VARIABLE,
    SYMBOL
};

typedef struct {
    size_t line;
    size_t character;
    const char *file;
} SourcePosition;

/**
 * A Token
 * @warning NEVER RELEASE A TOKEN!
 */
struct Token {
    TokenType type;
    EmojicodeString value;
    uint32_t valueLength;
    SourcePosition position;
    struct Token *nextToken;
    uint_fast32_t valueSize;
};

Token* newToken(Token *prevToken);

/**
 * Returns a token description string
 */
const char* tokenTypeToString(TokenType type);

bool tokenValueEqual(Token *a, Token *b);

/**
 * If @c token is not of type @c type a compiler error is thrown.
 */
void tokenTypeCheck(TokenType type, Token *token);

struct Variable {
    Variable(Token *n, Type t) : name(n), type(t) {}
    
    /** The name of the variable */
    Token *name;
    /** The ID of the variable */
    uint8_t id;
    /** The type */
    Type type;
};

/*
 * MARK: Classes
 */

/** Class */
class Class {
public:
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar enamespace;
    /** Whether this eclass eligible for initializer inheritance */
    bool inheritsContructors;
    
    /** The eclass' superclass. NULL if the eclass has no superclass. */
    struct Class *superclass;
    
    /** The offset for the instance variable IDs. The first instance variable will receive the value of this field. */
    uint16_t IDOffset;
    
    uint16_t index;
    
    /** The variable names. */
    std::vector<Variable> instanceVariables;
    /** Deinitializer for primitive types */
    void (*deconstruct)(void *);
    
    /** List of all methods for user classes */
    std::vector<Method *> methodList;
    std::vector<Initializer *> initializerList;
    std::vector<ClassMethod *> classMethodList;
    std::vector<Initializer *> requiredInitializerList;
    std::vector<Protocol *> protocols;
    
    Token *classBegin;
    Token *documentationToken;
    
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
    Enum(EmojicodeChar name, Package& package, Token *dt) : name(name), package(package), documentationToken(dt) {}
    
    EmojicodeChar name;
    std::map<EmojicodeChar, EmojicodeInteger> *dictionary;
    /** The package in which this eclass was defined. */
    Package& package;
    
    Token *documentationToken;
};

extern Enum* newEnum(EmojicodeChar name, EmojicodeChar enamespace);
extern void enumAddValue(EmojicodeChar name, Enum *eenum, EmojicodeInteger value);
extern Enum* getEnum(EmojicodeChar name, EmojicodeChar enamespace);
extern EmojicodeInteger* enumGetValue(EmojicodeChar name, Enum *eenum);

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
    
    Token *documentationToken;
};

void packageRegisterHeaderNewest(const char *name, EmojicodeChar enamespace);

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_ENUMERATOR;

extern std::map<std::array<EmojicodeChar, 2>, Class*> classesRegister;
extern std::map<std::array<EmojicodeChar, 2>, Protocol*> protocolsRegister;
extern std::map<std::array<EmojicodeChar, 2>, Enum*> enumsRegister;

extern Protocol *PR_ENUMERATEABLE;

//MARK: Static Analyzation

struct CompilerVariable {
public:
    CompilerVariable(Type type, uint8_t id, bool initd, bool frozen) : type(type), initialized(initd), id(id), frozen(frozen) {};
    /** The type of the variable. **/
    Type type;
    /** The ID of the variable. */
    uint8_t id;
    /** The variable is initialized if this field is greater than 0. */
    int initialized;
    /** Set for instance variables. */
    Variable *variable;
    /** Indicating whether variable was frozen. */
    bool frozen;
    
    /** Throws an error if the variable is not initalized. */
    void uninitalizedError(Token *variableToken) const;
    /** Throws an error if the variable is frozen. */
    void frozenError(Token *variableToken) const;
};


//MARK: Errors

/**
 * Throws a compiler error and exits the compiler.
 * @param token Used to determine the error location. If @c NULL the error origin is the beginning of the document.
 */

_Noreturn void compilerError(Token *token, const char *err, ...);

/**
 * Throws a compiler warning.
 * @param token Used to determine the error location. If @c NULL the error origin is the beginning of the document.
 */
void compilerWarning(Token *token, const char *err, ...);

/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);


//MARK: Lexer

Token* lex(FILE *f, const char* fileName);


//MARK: Protocol

/** Returns the protocol with name @c name in enamespace @c namepsace or @c NULL if the protocol cannot be found. */
extern Protocol* getProtocol(EmojicodeChar name, EmojicodeChar enamespace);

/** @warning Do not try to execute a method of a protocol! */
extern Method* protocolGetMethod(EmojicodeChar name, Protocol *protocol);

extern Token* currentToken;
extern Token* consumeToken();
#define nextToken() (currentToken->nextToken)

void report(const char *packageName);

#endif
