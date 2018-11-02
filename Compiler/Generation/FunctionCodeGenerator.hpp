//
//  FunctionCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionCodeGenerator_hpp
#define FunctionCodeGenerator_hpp

#include "CodeGenerator.hpp"
#include "Scoping/IDScoper.hpp"
#include <llvm/IR/IRBuilder.h>
#include <functional>
#include <queue>

namespace EmojicodeCompiler {

class Compiler;
class Function;

/// A FunctionCodeGenerator instance is responsible for generating the LLVM IR for a single function.
///
/// It manages the declaration of arguments, the context of the function, the scoper and is responsible for keeping
/// track of temporary objects.
///
/// This class provides helper methods to simplify use of the LLVM API and for common actions inside the compiler’s
/// code base, as for example retrieving the class info from an object.
class FunctionCodeGenerator {
public:
    /// Constructs a FunctionCodeGenerator.
    FunctionCodeGenerator(Function *function, llvm::Function *llvmFunc, CodeGenerator *generator);

    /// Constructs a FunctionCodeGenerator for generating a function that does not have corresponding Function.
    /// @see createEntry()
    FunctionCodeGenerator(llvm::Function *llvmFunc, CodeGenerator *generator)
        : fn_(nullptr), function_(llvmFunc), scoper_(0), generator_(generator), builder_(generator->context()) {}

    /// Generates the code for the provided Function.
    /// @pre A Function must have been provided to the constructor.
    void generate();

    /// Creates the entry block and initializes the builder to it.
    void createEntry();

    CGScoper& scoper() { return scoper_; }
    Compiler* compiler() const;
    CodeGenerator* generator() const { return generator_; }
    llvm::IRBuilder<>& builder() { return builder_; }
    LLVMTypeHelper& typeHelper() { return generator()->typeHelper(); }
    virtual llvm::Value* thisValue() const { return &*function_->args().begin(); }
    llvm::Type* llvmReturnType() const { return function_->getReturnType(); }

    llvm::Value* instanceVariablePointer(size_t id);

    /// @returns The number of bytes an instance of @c type takes up in memory.
    /// @see sizeOfReferencedType
    llvm::Value* sizeOf(llvm::Type *type);
    /// @returns The number of bytes an instance of the type pointed to by a pointer of @c ptrType takes up.
    ///          Behaves like `sizeOf(ptrType->getElementType())`, although this method does not call sizeOf().
    /// @see sizeOf
    llvm::Value* sizeOfReferencedType(llvm::PointerType *ptrType);

    /// Gets a pointer to the box info field of a box.
    /// @param box Pointer to a box.
    llvm::Value* buildGetBoxInfoPtr(llvm::Value *box);
    /// @see buildGetBoxValuePtr
    /// @param type The type is converted to a type with LLVMTypeHelper. The value pointer is then casted to
    ///             a pointer of this type.
    llvm::Value* buildGetBoxValuePtr(llvm::Value *box, const Type &type);
    /// Gets a pointer to the value field of a box as `llvmType`.
    /// @param box Pointer to a box.
    /// @param llvmType The type to which the pointer to the field is cast. Must be a pointer type.
    llvm::Value* buildGetBoxValuePtr(llvm::Value *box, llvm::Type *llvmType);
    /// Gets a pointer to the value field of a box as `llvmType`.
    /// @param box Pointer to a box.
    /// @param llvmType The type to which the pointer to the field is cast. Must be a pointer type.
    llvm::Value* buildGetBoxValuePtrAfter(llvm::Value *box, llvm::Type *llvmType, llvm::Type *after);
    /// Makes the box contain no value.
    /// @param box Pointer to a box.
    llvm::Value* buildMakeNoValue(llvm::Value *box);

    llvm::Value* buildOptionalHasValue(llvm::Value *simpleOptional);
    llvm::Value* buildOptionalHasValuePtr(llvm::Value *simpleOptional);
    llvm::Value* buildGetOptionalValuePtr(llvm::Value *simpleOptional);
    llvm::Value* buildHasNoValue(llvm::Value *simpleOptional);
    llvm::Value* buildHasNoValueBox(llvm::Value *box);
    llvm::Value* buildHasNoValueBoxPtr(llvm::Value *box);
    llvm::Value* buildSimpleOptionalWithoutValue(const Type &type);
    llvm::Value* buildBoxOptionalWithoutValue();
    llvm::Value* buildSimpleOptionalWithValue(llvm::Value *value, const Type &type);
    llvm::Value* buildGetOptionalValue(llvm::Value *value, const Type &type);

    /// Gets a pointer to the pointer to the class info of an object.
    /// @see getClassInfoFromObject
    llvm::Value* buildGetClassInfoPtrFromObject(llvm::Value *object);
    /// Gets a pointer to the class info of an object.
    /// @param object Pointer to the object from which the class info shall be obtained.
    /// @returns A llvm::Value* representing a pointer to a class info.
    llvm::Value* buildGetClassInfoFromObject(llvm::Value *object);

    llvm::Value* buildGetErrorNoError() { return int64(-1); }
    llvm::Value* buildGetIsError(llvm::Value *simpleError);
    llvm::Value* buildGetIsNotError(llvm::Value *simpleError);
    llvm::Value* buildErrorIsNotErrorPtr(llvm::Value *simpleErrorPtr);
    llvm::Value* buildGetErrorValuePtr(llvm::Value *simpleErrorPtr);
    llvm::Value* buildSimpleErrorWithError(llvm::Value *errorEnumValue, llvm::Type *type);
    llvm::Value* buildErrorEnumValueBoxPtr(llvm::Value *box, const Type &type);

    llvm::Value* buildFindProtocolConformance(llvm::Value *box, llvm::Value *boxInfo, llvm::Value *protocolIdentifier);

    llvm::Value* int8(int8_t value);
    llvm::Value* int16(int16_t value);
    llvm::Value* int32(int32_t value);
    llvm::Value* int64(int64_t value);

    llvm::Value* boxInfoFor(const Type &type);

    /// Allocates heap memory using the runtime library’s ejcAlloc.
    ///
    /// Allocates enough bytes to hold the element type of the pointer type `type`.
    ///
    /// @note ejcAlloc expects the first element of the allocated type to be a pointer to the control
    /// block.
    llvm::Value* alloc(llvm::PointerType *type);
    /// Allocates stack memory as replacement for a heap memory allocation as performed by alloc().
    ///
    /// In order to ensure compatibility with the runtime library’s retain and release functions, additional bytes
    /// are allocated in front of the object.
    ///
    /// @note Like ejcAlloc, this function expects the first element of the allocated type to be a pointer to the
    /// control block.
    llvm::Value* stackAlloc(llvm::PointerType *type);

    llvm::Value* managableGetValuePtr(llvm::Value *managablePtr);

    void release(llvm::Value *value, const Type &type);
    /// Like release() but the value to be released is always provided as a pointer. If isManagedByReference() returns
    /// fales for `type`, the value is loaded before it is passed to release().
    /// @param ptr Pointer to the value to be released.
    void releaseByReference(llvm::Value *ptr, const Type &type);
    void retain(llvm::Value *value, const Type &type);
    bool isManagedByReference(const Type &type) const;

    llvm::Value* createEntryAlloca(llvm::Type *type, const llvm::Twine &name = "");

    /// Creates an if-else branch condition. If the condition evaluates to true, the code produces by the @c then
    /// function is executed, otherwise the code produced by @c otherwise.
    void createIfElse(llvm::Value* cond, const std::function<void()> &then, const std::function<void()> &otherwise);
    /// Like createIfElse() but the if and else block only branch to a continue block if the respective functions
    /// return true.
    void createIfElseBranchCond(llvm::Value* cond, const std::function<bool()> &then,
                                const std::function<bool()> &otherwise);
    /// Creates an if block. The code produced by the @then function is only executed if the condition is true.
    void createIf(llvm::Value* cond, const std::function<void()> &then);

    llvm::Value* createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
                                 const std::function<llvm::Value *()> &otherwise);

    using PairIfElseCallback = std::function<std::pair<llvm::Value*, llvm::Value*> ()>;
    std::pair<llvm::Value*, llvm::Value*> createIfElsePhi(llvm::Value* cond, const PairIfElseCallback &then,
                                                          const PairIfElseCallback &otherwise);

    /// Registers a temporary value that must be released at the end of the statement.
    /// @param value The value that must be destroyed.
    /// @param type The type of the value.
    void addTemporaryObject(llvm::Value *value, const Type &type) {
        temporaryObjects_.emplace(value, type);
    }
    /// Releases all temporary values that were previously registered with addTemporaryObject() in the order
    /// they were added.
    /// @see addTemporaryObject
    void releaseTemporaryObjects();

protected:
    virtual void declareArguments(llvm::Function *function);
    Function* function() const { return fn_; }

private:
    struct Temporary {
        Temporary(llvm::Value *value, Type type) : value(value), type(type) {}
        llvm::Value *value;
        Type type;
    };

    Function *const fn_;
    llvm::Function *const function_;
    CGScoper scoper_;

    CodeGenerator *const generator_;
    llvm::IRBuilder<> builder_;

    std::queue<Temporary> temporaryObjects_;

    /// @param retain True if the box should be released, false if it should be retained.
    void manageBox(bool retain, llvm::Value *boxInfo, llvm::Value *value, const Type &type);

    void addParamAttrs(const Type &argType, llvm::Argument &llvmArg);
};

}  // namespace EmojicodeCompiler

#endif /* FunctionCodeGenerator_hpp */
