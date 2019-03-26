//
//  Mangler.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Mangler_hpp
#define Mangler_hpp

#include <map>
#include <string>

namespace EmojicodeCompiler {

class Function;
class Type;
class Class;
class Type;
class TypeDefinition;

std::string mangleFunction(Function *function, const std::map<size_t, Type> &genericArgs);
std::string mangleTypeName(const Type &type);
std::string mangleClassInfoName(Class *klass);
std::string mangleBoxInfoName(const Type &type);
std::string mangleProtocolConformance(const Type &type, const Type &protocol);
std::string mangleProtocolRunTimeTypeInfo(const Type &type);
std::string mangleMultiprotocolConformance(const Type &multi, const Type &conformer);
std::string mangleBoxRetain(const Type &type);
std::string mangleBoxRelease(const Type &type);

}  // namespace EmojicodeCompiler

#endif /* Mangler_hpp */
