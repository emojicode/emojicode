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

typedef struct Class Class;
typedef struct Method Method;
typedef struct ClassMethod ClassMethod;
typedef struct Initializer Initializer;
typedef struct Protocol Protocol;
typedef struct Enum Enum;
typedef struct Scope Scope;
typedef struct ScopeWrapper ScopeWrapper;
typedef struct List List;
typedef struct Dictionary Dictionary;
typedef struct Token Token;

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_ENUMERATOR;

extern Dictionary *classesRegister;
extern Dictionary *protocolsRegister;
extern Dictionary *enumsRegister;

extern Protocol *PR_ENUMERATEABLE;

List *classes;
List *packages;

#include "Type.h"

/** String */
typedef struct String {
    /** The number of code points in @c characters. Strings are not null terminated! */
    uint_fast32_t length;
    /** The characters of this string. Strings are not null terminated! */
    EmojicodeChar *characters;
} String;

typedef struct {
    Class *class;
    ClassMethod *method;
} StartingFlag;
StartingFlag startingFlag;
bool foundStartingFlag;

typedef struct {
    const char *name;
    PackageVersion version;
    bool requiresNativeBinary;
} Package;

typedef enum {
    PUBLIC, PRIVATE, PROTECTED
} AccessLevel;



//MARK: Tokens

typedef enum {
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
} TokenType;

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
    EmojicodeChar *value;
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

/** Arguments for a class or initializer */
typedef struct Arguments {
    /** The name of the variables to store the arguments in respective order */
    Variable *variables;
    /** The number of arguments */
    uint8_t count;
} Arguments;

/*
 * MARK: Classes
 */

/** Class */
struct Class {
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar namespace;
    /** Whether this class eligible for initializer inheritance */
    bool inheritsContructors;
    
    /** Hashmap holding methods. @warning Don't access it directly, use the correct functions. */
    Dictionary *methods;
    /** Hashmap holding class methods. @warning Don't access it directly, use the correct functions. */
    Dictionary *classMethods;
    /** Hashmap holding initializers. @warning Don't access it directly, use the correct functions. */
    Dictionary *initializers;
    /** The class' superclass. NULL if the class has no superclass. */
    struct Class *superclass;
    
    /** The number of instance variables. */
    uint16_t instanceVariableCount;
    uint16_t instanceVariablesSize;
    
    /** The offset for the instance variable IDs. The first instance variable will receive the value of this field. */
    uint16_t IDOffset;
    
    uint16_t index;
    
    /** The variable names. */
    Variable **instanceVariables;
    /** Deinitializer for primitive types */
    void (*deconstruct)(void *);
    
    /** List of all methods for user classes */
    List *methodList;
    List *initializerList;
    List *classMethodList;
    List *requiredInitializerList;
    List *protocols;
    
    Token *classBegin;
    Token *documentationToken;
    
    uint16_t nextMethodVti;
    uint16_t nextClassMethodVti;
    uint16_t nextInitializerVti;
    
    /** The number of generic arguments including those from a super class. */
    uint16_t genericArgumentCount;
    /** The number of generic arguments this class takes. */
    uint16_t ownGenericArgumentCount;
    /** The types for the generic arguments. */
    Type *genericArgumentContraints;
    /** The arguments for the classes from which this class inherits. */
    Type *superGenericArguments;
    /** Generic type arguments as variables */
    Dictionary *ownGenericArgumentVariables;
    
    /** The package in which this class was defined. */
    Package *package;
};

/** Before creating or using any types call this function */
void initTypes();

/** Fetch a class by its name and namespace. Returns NULL if the class cannot be found. */
extern Class* getClass(EmojicodeChar name, EmojicodeChar namespace);

/** Returns a method of object by name */
extern Method* getMethod(EmojicodeChar name, Class *);

/** Gets a initializer by its name returns @c NULL if the class does not have an initializer with this name */
extern Initializer* getInitializer(EmojicodeChar name, Class *class);

/** Returns a method of object by name */
extern ClassMethod* getClassMethod(EmojicodeChar name, Class *);


//MARK: Protocols

/** Creates a new protocol */
extern Protocol* newProtocol(EmojicodeChar name, EmojicodeChar namespace);

/** Adds a method to a protocol. The parameters have the same meaning as in @c addMethod. */
extern Method* protocolAddMethod(EmojicodeChar name, Protocol *protocol, Arguments arguments, Type returnType);

struct Enum {
    EmojicodeChar name;
    Dictionary *dictionary;
    /** The package in which this class was defined. */
    Package *package;
    
    Token *documentationToken;
};

extern Enum* newEnum(EmojicodeChar name, EmojicodeChar namespace);
extern void enumAddValue(EmojicodeChar name, Enum *eenum, EmojicodeInteger value);
extern Enum* getEnum(EmojicodeChar name, EmojicodeChar namespace);
extern EmojicodeInteger* enumGetValue(EmojicodeChar name, Enum *eenum);

struct Protocol {
    /** Self explaining */
    EmojicodeChar name;
    /** Self explaining */
    EmojicodeChar namespace;
    
    /** List of all methods. */
    List *methodList;
    
    /** Hashmap holding methods. @warning Don't access it directly, use the correct functions. */
    Dictionary *methods;
    
    uint_fast16_t index;
    
    /** The package in which this class was defined. */
    Package *package;
    
    Token *documentationToken;
};

typedef struct {
    /** The procedure name. A Unicode code point for an emoji */
    EmojicodeChar name;
    /** Argument list */
    Arguments arguments;
    
    /** Whether the method is native */
    bool native;
    
    bool final : 1;
    bool overriding : 1;
    
    AccessLevel access;
    /** Class which defined this method */
    Class *class;
    
    /** Token at which this method was defined */
    Token *dToken;
    Token *documentationToken;
    
    uint16_t vti;
    
    /** Return type of the method */
    Type returnType;
    
    Token *firstToken;
    
    EmojicodeChar namespace;
} Procedure;

struct Method {
    Procedure pc;
};

struct ClassMethod {
    Procedure pc;
};

struct Initializer {
    Procedure pc;
    bool required : 1;
    bool canReturnNothingness : 1;
};


void packageRegisterHeaderNewest(const char *name, EmojicodeChar namespace);


/** Returns true if @c a or a superclass of @c a conforms to @c to. */
extern bool conformsTo(Class *a, Protocol *to);

/** Returns true if @c a inherits from class @c from */
extern bool inheritsFrom(Class *a, Class *from);


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

/** Returns the protocol with name @c name in namespace @c namepsace or @c NULL if the protocol cannot be found. */
extern Protocol* getProtocol(EmojicodeChar name, EmojicodeChar namespace);

/** @warning Do not try to execute a method of a protocol! */
extern Method* protocolGetMethod(EmojicodeChar name, Protocol *protocol);

/// Releases an Arguments structure
void releaseArgumentsStructure(Arguments *args);

extern Token* currentToken;
extern Token* consumeToken();
#define nextToken() (currentToken->nextToken)

//MARK: Compiler List

List* newList();
void appendList(List *list, void* o);
void* getList(List *list, size_t i);
bool listRemoveByIndex(List *list, size_t index);
List* listFromList(List *cpdList);
bool listRemove(List *list, void *x);
void listRelease(void *l);
void insertList(List *list, void *o, size_t index);

struct List {
    size_t count;
    size_t size;
    void **items;
};

char* stringToChar(String *str);

typedef struct DictionaryKVP {
    void *key;
    size_t kl;
    void *value;
} DictionaryKVP;

struct Dictionary {
    DictionaryKVP *slots;
    size_t capacity;
    size_t count;
};

Dictionary* newDictionary();
void dictionarySet(Dictionary *dict, const void *key, size_t kl, void *value);
void* dictionaryLookup(Dictionary *dict, const void *key, size_t kl);
void dictionaryRemove(Dictionary *dict, const void *key, size_t kl);
void dictionaryFree(Dictionary *dict, void(*fr)(void *));

void report(const char *packageName);

#endif
