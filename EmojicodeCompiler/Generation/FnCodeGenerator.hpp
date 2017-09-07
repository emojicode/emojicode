//
//  FnCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FnCodeGenerator_hpp
#define FnCodeGenerator_hpp

#include <llvm/IR/IRBuilder.h>
#include "../Functions/Function.hpp"
#include "../Scoping/CGScoper.hpp"
#include "../Package/Package.hpp"
#include "CodeGenerator.hpp"
#include <functional>

namespace EmojicodeCompiler {

class Application;

struct LocalVariable {
    LocalVariable() = default;
    LocalVariable(bool isMutable, llvm::Value *value) : isMutable(isMutable), value(value) {}
    bool isMutable = true;
    llvm::Value *value = nullptr;
};

class FnCodeGenerator {
public:
    explicit FnCodeGenerator(Function *function, CodeGenerator *generator)
    : fn_(function), scoper_(function->variableCount()),  generator_(generator), builder_(generator->context()) {}
    void generate();

    CGScoper<LocalVariable>& scoper() { return scoper_; }
    Application* app() { return generator()->package()->app(); }
    CodeGenerator* generator() { return generator_; }
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
    llvm::Value* getObjectMetaPtr(llvm::Value *object);
    llvm::Value* getMakeNoValue(llvm::Value *box);

    void createIfElse(llvm::Value* cond, const std::function<void()> &then, const std::function<void()> &otherwise);
    llvm::Value* createIfElsePhi(llvm::Value* cond, const std::function<llvm::Value* ()> &then,
                                 const std::function<llvm::Value *()> &otherwise);
protected:
    virtual void declareArguments(llvm::Function *function);
private:
    Function *fn_;
    CGScoper<LocalVariable> scoper_;

    CodeGenerator *generator_;
    llvm::IRBuilder<> builder_;
};

}  // namespace EmojicodeCompiler

#endif /* FnCodeGenerator_hpp */
