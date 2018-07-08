//
//  MFHeapAllocates.hpp
//  runtime
//
//  Created by Theo Weidmann on 24.06.18.
//

#ifndef MFHeapAllocates_hpp
#define MFHeapAllocates_hpp

#include "MFType.hpp"

namespace llvm {
class Type;
class Value;
}

namespace EmojicodeCompiler {

class FunctionCodeGenerator;

/// An AST expression node that allocates memory should inherit from this subclass to be informed when allocation on
/// stack is possible instead of heap allocation (as determined by flow analysis).
class MFHeapAllocates {
public:
    /// Called if memory flow analysis determined that the value can be placed on the stack.
    virtual void allocateOnStack() = 0;
};

/// A class that would either simply call FunctionCodeGenerator::alloc or FunctionCodeGenerator::createEntryAlloca
/// depending on whether allocateOnStack() was called, should inherit from this class instead, which encapsulates this
/// logic and provides allocate() for allocation.
class MFHeapAutoAllocates : public MFHeapAllocates {
public:
    void allocateOnStack() final {
        stack_ = true;
    }

    void analyseAllocation(MFType type);

    /// Allocates either on the heap or on the stack.
    /// @param type The type of the object that shall be allocated. Note that this does not have to be a pointer.
    ///             If this is a pointer, size appropriate for the pointer itself is reserved.
    llvm::Value* allocate(FunctionCodeGenerator *fg, llvm::Type *type) const;
private:
    bool stack_ = false;
};

}  // namespace EmojicodeCompiler

#endif /* MFHeapAllocates_hpp */
