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
class TypeDefinitionFunctional;
class Class;
class Protocol;
class Enum;
class Token;
class Type;
class Package;
class CompilerErrorException;
class ValueType;

struct SourcePosition;

class EmojicodeString: public std::basic_string<EmojicodeChar>  {
public:
    char* utf8CString() const;
};

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_RANGE;
extern Protocol *PR_ENUMERATOR;
extern Protocol *PR_ENUMERATEABLE;
extern ValueType *VT_BOOLEAN;
extern ValueType *VT_SYMBOL;
extern ValueType *VT_INTEGER;
extern ValueType *VT_DOUBLE;

/** Issues a compiler warning. The compilation is continued afterwards. */
void compilerWarning(SourcePosition p, const char *err, ...);
/** Prints the given error and stores that an error was raised during compilation. */
void printError(const CompilerErrorException &ce);
/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);

#endif /* EmojicodeCompiler_hpp */
