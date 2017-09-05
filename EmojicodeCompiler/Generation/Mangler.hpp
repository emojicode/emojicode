//
//  Mangler.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Mangler_hpp
#define Mangler_hpp

#include <string>

namespace EmojicodeCompiler {

class Function;
class Type;
class Class;

std::string mangleFunctionName(Function *function);
std::string mangleTypeName(const Type &type);
std::string mangleClassMetaName(Class *klass);

}  // namespace EmojicodeCompiler

#endif /* Mangler_hpp */
