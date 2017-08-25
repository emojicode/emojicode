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
#include <sstream>
#include <string>
#include <codecvt>
#include <locale>

namespace EmojicodeCompiler {

class Class;
class Protocol;
class Type;
class Package;
class CompilerError;
class ValueType;

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

inline std::string utf8(const std::u32string &s) {
    return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().to_bytes(s);
}

template<typename Head>
void appendToStream(std::stringstream &stream, Head head) {
    stream << head;
}

template<typename Head, typename... Args>
void appendToStream(std::stringstream &stream, Head head, Args... args) {
    stream << head;
    appendToStream(stream, args...);
}

}  // namespace EmojicodeCompiler

#endif /* EmojicodeCompiler_hpp */
