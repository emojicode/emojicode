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
    if (stack_) {
        return fg->createEntryAlloca(type);
    }

    return fg->alloc(type->getPointerTo());
}

void MFHeapAutoAllocates::analyseAllocation(MFType type) {
    if (type == MFType::Borrowing) {
        allocateOnStack();
    }
}

}  // namespace EmojicodeCompiler
