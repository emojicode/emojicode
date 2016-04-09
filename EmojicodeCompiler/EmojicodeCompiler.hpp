//
//  EmojicodeCompiler.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeCompiler_hpp
#define EmojicodeCompiler_hpp

#define EmojicodeCompiler

#include "EmojicodeShared.h"
#include "Emojis.h"

#include <vector>
#include <map>
#include <string>

class TypeDefinition;
class TypeDefinitionWithGenerics;
class Class;
class Protocol;
class Enum;
class Token;

class Method;
class Initializer;
class ClassMethod;

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

struct Variable {
    Variable(const Token *n, Type t) : name(n), type(t) {}
    
    /** The name of the variable */
    const Token *name;
    /** The type */
    Type type;
};

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_ENUMERATOR;
extern Class *CL_RANGE;
extern Protocol *PR_ENUMERATEABLE;

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

void report(Package *package);

#endif /* EmojicodeCompiler_hpp */
