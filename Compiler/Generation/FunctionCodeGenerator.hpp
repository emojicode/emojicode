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
#include "Functions/Function.hpp"
#include "Scoping/IDScoper.hpp"
#include <llvm/IR/IRBuilder.h>
#include <functional>
#include <queue>

namespace EmojicodeCompiler {

class Compiler;

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
    FunctionCodeGenerator(Function *function, llvm::Function *llvmFunc, CodeGenerator *generator)
        : fn_(function), function_(llvmFunc), scoper_(function->variableCount()),
          generator_(generator), builder_(generator->context()) {}

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

    /// @returns The number of bytes an instance of @c type takes up in memory.
    /// @see sizeOfReferencedType
    llvm::Value* sizeOf(llvm::Type *type);
    /// @returns The number of bytes an instance of the type pointed to by a pointer of @c ptrType takes up.
    ///          Behaves like `sizeOf(ptrType->getElementType())`, although this method does not call sizeOf().
    /// @see sizeOf
    llvm::Value* sizeOfReferencedType(llvm::PointerType *ptrType);
    /// Gets a pointer to the box info field of box.
    /// @param box A pointer to a box.
    llvm::Value* getBoxInfoPtr(llvm::Value *box);
    llvm::Value* getValuePtr(llvm::Value *box, const Type &type);
    llvm::Value* getValuePtr(llvm::Value *box, llvm::Type *llvmType);
    llvm::Value* getMakeNoValue(llvm::Value *box);

    llvm::Value* buildOptionalHasValue(llvm::Value *simpleOptional);
    llvm::Value* buildOptionalHasValuePtr(llvm::Value *simpleOptional);
    llvm::Value* buildGetOptionalValuePtr(llvm::Value *simpleOptional);
    llvm::Value* getHasNoValue(llvm::Value *simpleOptional);
    llvm::Value* getHasNoValueBox(llvm::Value *box);
    llvm::Value* getHasNoValueBoxPtr(llvm::Value *box);
    llvm::Value* getSimpleOptionalWithoutValue(const Type &type);
    llvm::Value* getBoxOptionalWithoutValue();
    llvm::Value* getSimpleOptionalWithValue(llvm::Value *value, const Type &type);

    /// Gets a pointer to the pointer to the class info of an object.
    /// @see getClassInfoFromObject
    llvm::Value* getClassInfoPtrFromObject(llvm::Value *object);
    /// Gets a pointer to the class info of an object.
    /// @param object Pointer to the object from which the class info shall be obtained.
    /// @returns A llvm::Value* representing a pointer to a class info.
    llvm::Value* getClassInfoFromObject(llvm::Value *object);

    llvm::Value* getErrorNoError() { return int64(-1); }
    llvm::Value* getIsError(llvm::Value *simpleError);
    llvm::Value* getSimpleErrorWithError(llvm::Value *errorEnumValue, llvm::Type *type);
    llvm::Value* getErrorEnumValueBoxPtr(llvm::Value *box, const Type &type);

    llvm::Value* int8(int8_t value);
    llvm::Value* int16(int16_t value);
    llvm::Value* int32(int32_t value);
    llvm::Value* int64(int64_t value);

    llvm::Value* boxInfoFor(const Type &type);

    llvm::Value* alloc(llvm::PointerType *type);
    void release(llvm::Value *value, const Type &type, bool temporary);
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

    void addTemporaryObject(llvm::Value *value, const Type &type, bool local) {
        temporaryObjects_.emplace(value, type, local);
    }
    void releaseTemporaryObjects();

protected:
    virtual void declareArguments(llvm::Function *function);
    Function* function() const { return fn_; }

private:
    struct Temporary {
        Temporary(llvm::Value *value, Type type, bool local) : value(value), type(type), local(local) {}
        llvm::Value *value;
        Type type;
        bool local;
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
