//
//  FunctionCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FunctionCodeGenerator_hpp
#define FunctionCodeGenerator_hpp

#include <llvm/IR/IRBuilder.h>
#include "../Functions/Function.hpp"
#include "../Scoping/CGScoper.hpp"
#include "CodeGenerator.hpp"
#include <functional>

namespace EmojicodeCompiler {

class Application;

/// A FunctionCodeGenerator instance is responsible for generating the LLVM IR for a single function.
class FunctionCodeGenerator {
public:
    /// Constructs a FunctionCodeGenerator.
    /// @param function The function whose code shall be generated.
    ///                 The function must have a value for Function::llvmFunction().
    FunctionCodeGenerator(Function *function, CodeGenerator *generator)
    : fn_(function), scoper_(function->variableCount()),  generator_(generator), builder_(generator->context()) {}
    void generate();

    CGScoper& scoper() { return scoper_; }
    Application* app() const;
    CodeGenerator* generator() const { return generator_; }
    llvm::IRBuilder<>& builder() { return builder_; }
    LLVMTypeHelper& typeHelper() { return generator()->typeHelper(); }
    llvm::Value* thisValue() { return &*fn_->llvmFunction()->args().begin(); }

    llvm::Value* sizeFor(llvm::PointerType *type);
    llvm::Value* getMetaFromObject(llvm::Value *object);
    llvm::Value* getHasBoxNoValue(llvm::Value *box);
    /// Gets a pointer to the meta type field of box.
    /// @param box A pointer to a box.
    llvm::Value* getMetaTypePtr(llvm::Value *box);
    llvm::Value* getHasNoValue(llvm::Value *simpleOptional);
    llvm::Value* getSimpleOptionalWithoutValue(const Type &type);
    llvm::Value* getSimpleOptionalWithValue(llvm::Value *value, const Type &type);
    llvm::Value* getValuePtr(llvm::Value *box, const Type &type);
    llvm::Value* getValuePtr(llvm::Value *box, llvm::Type *llvmType);
    llvm::Value* getObjectMetaPtr(llvm::Value *object);
    llvm::Value* getMakeNoValue(llvm::Value *box);

    void createIfElse(llvm::Value* cond, const std::function<void()> &then, const std::function<void()> &otherwise);
    llvm::Value* createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
                                 const std::function<llvm::Value *()> &otherwise);

    using PairIfElseCallback = std::function<std::pair<llvm::Value*, llvm::Value*> ()>;
    std::pair<llvm::Value*, llvm::Value*> createIfElsePhi(llvm::Value* cond, const PairIfElseCallback &then,
                                                          const PairIfElseCallback &otherwise);
protected:
    virtual void declareArguments(llvm::Function *function);
private:
    Function *fn_;
    CGScoper scoper_;

    CodeGenerator *generator_;
    llvm::IRBuilder<> builder_;
};

}  // namespace EmojicodeCompiler

#endif /* FunctionCodeGenerator_hpp */
