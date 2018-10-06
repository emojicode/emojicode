//
//  MFHeapAllocates.cpp
//  runtime
//
//  Created by Theo Weidmann on 24.06.18.
//

#include "MFHeapAllocates.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

llvm::Value* MFHeapAutoAllocates::allocate(FunctionCodeGenerator *fg, llvm::Type *type) const {
   return stack_ ? fg->stackAlloc(type->getPointerTo()) : fg->alloc(type->getPointerTo());
}

void MFHeapAutoAllocates::analyseAllocation(MFFlowCategory type) {
    if (!type.isEscaping()) {
        allocateOnStack();
    }
}

}  // namespace EmojicodeCompiler
