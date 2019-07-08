//
//  BoxRetainReleaseBuilder.hpp
//  runtime
//
//  Created by Theo Weidmann on 30.03.19.
//

#ifndef BoxRetainReleaseBuilder_hpp
#define BoxRetainReleaseBuilder_hpp

#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

class CodeGenerator;
class Type;
class ValueType;
class TypeDefinition;

std::pair<llvm::Function*, llvm::Function*> buildBoxRetainRelease(CodeGenerator *cg, const Type &type); 
void buildCopyRetain(CodeGenerator *cg, ValueType *typeDef);
void buildDestructor(CodeGenerator *cg, TypeDefinition *typeDef);
llvm::Function* createMemoryFunction(const std::string &str, CodeGenerator *cg, TypeDefinition *typeDef);

}

#endif /* BoxRetainReleaseBuilder_hpp */
