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

#include <string>

class TypeDefinition;
class TypeDefinitionWithGenerics;
class Class;
class Protocol;
class Enum;
class Token;
class Type;
class Package;

class Method;
class Initializer;
class ClassMethod;

struct SourcePosition;

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

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_RANGE;
extern Protocol *PR_ENUMERATOR;
extern Protocol *PR_ENUMERATEABLE;

//MARK: Errors

/**
 * Issues a compiler error and exits the compiler.
 */
[[noreturn]] void compilerError(SourcePosition p, const char *err, ...);

/**
 * Issues a compiler warning. The compilation is continued afterwards.
 */
void compilerWarning(SourcePosition p, const char *err, ...);

/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);

void report(Package *package);

#endif /* EmojicodeCompiler_hpp */
