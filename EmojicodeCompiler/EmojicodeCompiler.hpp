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
class Scope;
class ScopeWrapper;
class Token;

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

/** Returns the protocol with name @c name in enamespace @c namepsace or @c nullptr if the protocol cannot be found. */
extern Enum* getEnum(EmojicodeChar name, EmojicodeChar enamespace);

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
 * @param token Used to determine the error location. If @c nullptr the error origin is the beginning of the document.
 */
[[noreturn]] void compilerError(const Token *token, const char *err, ...);

/**
 * Issues a compiler warning. The compilation is continued afterwards.
 * @param token Used to determine the error location. If @c nullptr the error origin is the beginning of the document.
 */
void compilerWarning(const Token *token, const char *err, ...);

/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);

void report(const char *packageName);

#endif
