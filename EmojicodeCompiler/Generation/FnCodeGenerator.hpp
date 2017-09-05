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
#include "../AST/ASTNode.hpp"
#include "../Functions/Function.hpp"
#include "../Scoping/CGScoper.hpp"
#include "../Application.hpp"
#include "CodeGenerator.hpp"
#include <memory>

namespace EmojicodeCompiler {

struct LocalVariable {
    LocalVariable() = default;
    LocalVariable(bool isMutable, llvm::Value *value) : isMutable(isMutable), value(value) {}
    bool isMutable;
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
    llvm::Value* thisValue() { return &*fn_->llvmFunction()->args().begin(); }

    llvm::Value* sizeFor(llvm::PointerType *type);
    llvm::Value* getMetaFromObject(llvm::Value *object);
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
