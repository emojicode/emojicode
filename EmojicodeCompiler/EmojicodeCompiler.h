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

#include <vector>
#include <map>
#include <string>

class Class;
class Method;
class Initializer;
class ClassMethod;
typedef struct Protocol Protocol;
typedef struct Enum Enum;
typedef struct Scope Scope;
typedef struct ScopeWrapper ScopeWrapper;

__attribute__((deprecated))
typedef struct List List;
__attribute__((deprecated))
typedef struct Dictionary Dictionary;
typedef struct Token Token;


std::vector<Class *> classes;

#include "Type.h"

/** String */
typedef struct String {
    /** The number of code points in @c characters. Strings are not null terminated! */
    uint_fast32_t length;
    /** The characters of this string. Strings are not null terminated! */
    EmojicodeChar *characters;
} String;

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
    std::basic_string<EmojicodeChar> value;
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

typedef struct Variable {
    /** The name of the variable */
    Token *name;
    /** The ID of the variable */
    uint8_t id;
    /** The type */
    Type type;
} Variable;

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
    std::map<EmojicodeChar*, Type> ownGenericArgumentVariables;
    
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

struct Enum {
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

class Procedure {
public:
    Procedure(EmojicodeChar name, AccessLevel level, bool final, Class *eclass,
              EmojicodeChar theNamespace, Token *dToken, bool overriding, Token *documentationToken) :
        name(name), access(level), eclass(eclass), enamespace(theNamespace), dToken(dToken), overriding(overriding), documentationToken(documentationToken), returnType(typeNothingness) {}
    
    /** The procedure name. A Unicode code point for an emoji */
    EmojicodeChar name;
   
    std::vector<Variable> arguments;
    
    /** Whether the method is native */
    bool native : 1;
    
    bool final : 1;
    bool overriding : 1;
    
    AccessLevel access;
    /** Class which defined this method */
    Class *eclass;
    
    /** Token at which this method was defined */
    Token *dToken;
    Token *documentationToken;
    
    uint16_t vti;
    
    /** Return type of the method */
    Type returnType;
    
    Token *firstToken;
    
    EmojicodeChar enamespace;
    
    template <typename T> void duplicateDeclarationCheck(std::map<EmojicodeChar, T> dict);
    
    /**
     * Check whether this procedure is breaking promises.
     */
    void checkPromises(Procedure *superProcedure, Type parentType);
private:
    const char *on;
};

class Method: public Procedure {
    using Procedure::Procedure;
    
    const char *on = "Method";
};

class ClassMethod: public Procedure {
    using Procedure::Procedure;
    
    const char *on = "Class Method";
};

class Initializer: public Procedure {
public:
    Initializer(EmojicodeChar name, AccessLevel level, bool final, Class *eclass, EmojicodeChar theNamespace,
                Token *dToken, bool overriding, Token *documentationToken, bool r, bool crn) : Procedure(name, level, final, eclass, theNamespace, dToken, overriding, documentationToken), required(r), canReturnNothingness(crn) {}
    
    bool required : 1;
    bool canReturnNothingness : 1;
    const char *on = "Initializer";
};


void packageRegisterHeaderNewest(const char *name, EmojicodeChar enamespace);

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_ENUMERATOR;

extern std::map<EmojicodeChar[2], Class*> classesRegister;
extern std::map<EmojicodeChar[2], Protocol*> protocolsRegister;
extern std::map<EmojicodeChar[2], Enum*> enumsRegister;

extern Protocol *PR_ENUMERATEABLE;

//MARK: Static Analyzation

typedef struct {
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
} CompilerVariable;


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

/**
 * Creates a deep representation of the given type. 
 * @returns A pointer to a string which must be free’d.
 */
char* typeToString(Type type, Type parentType, bool includeNs);

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
