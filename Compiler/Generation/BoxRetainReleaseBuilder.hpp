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

std::pair<llvm::Function*, llvm::Function*> buildBoxRetainRelease(CodeGenerator *cg, const Type &type);

}

#endif /* BoxRetainReleaseBuilder_hpp */
