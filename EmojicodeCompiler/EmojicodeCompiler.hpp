//
//  EmojicodeCompiler.h
//  Emojicode
//
//  Created by Theo Weidmann on 01.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeCompiler_hpp
#define EmojicodeCompiler_hpp

#define EmojicodeCompiler_hpp

#include "../EmojicodeShared.h"
#include "Emojis.h"
#include <sstream>
#include <string>

namespace EmojicodeCompiler {

class Class;
class Protocol;
class Type;
class Package;
class CompilerError;
class ValueType;

struct SourcePosition;

extern std::string packageDirectory;

using InstructionCount = unsigned int;

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Protocol *PR_ENUMERATOR;
extern Protocol *PR_ENUMERATEABLE;
extern ValueType *VT_BOOLEAN;
extern ValueType *VT_SYMBOL;
extern ValueType *VT_INTEGER;
extern ValueType *VT_DOUBLE;

std::string utf8(const std::u32string &s);

/** Issues a compiler warning. The compilation is continued afterwards. */
void compilerWarning(const SourcePosition &p, const std::string &warning);

template<typename Head>
void appendToStream(std::stringstream &stream, Head head) {
    stream << head;
}

template<typename Head, typename... Args>
void appendToStream(std::stringstream &stream, Head head, Args... args) {
    stream << head;
    appendToStream(stream, args...);
}

template<typename... Args>
void compilerWarning(const SourcePosition &p, Args... args) {
    std::stringstream stream;
    appendToStream(stream, args...);
    compilerWarning(p, stream.str());
}

/** Prints the given error and stores that an error was raised during compilation. */
void printError(const CompilerError &ce);
/** Prints the string as escaped JSON string to the given file. */
void printJSONStringToFile(const char *string, FILE *f);

}  // namespace EmojicodeCompiler

#endif /* EmojicodeCompiler_hpp */
