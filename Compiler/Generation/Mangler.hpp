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
class Protocol;
class TypeDefinition;
class ReificationContext;
template <typename Entity>
struct Reification;
struct TypeDefinitionReification;

std::string mangleFunction(Function *function, const ReificationContext &reificationContext);
std::string mangleTypeDefinition(TypeDefinition *typeDef, const Reification<TypeDefinitionReification> *reification);
std::string mangleClassInfoName(Class *klass);
std::string mangleProtocolConformance(const Type &type, const Type &protocol);

std::string mangleBoxInfoName(const Type &type);
std::string mangleProtocolIdentifier(const Type &type);
std::string mangleMultiprotocolConformance(const Type &multi, const Type &conformer);
std::string mangleBoxRetain(const Type &type);
std::string mangleBoxRelease(const Type &type);

}  // namespace EmojicodeCompiler

#endif /* Mangler_hpp */
