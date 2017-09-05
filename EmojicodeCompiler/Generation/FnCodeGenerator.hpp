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
#include "../../EmojicodeInstructions.h"
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

//    void writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
//                                            EmojicodeInstruction object, EmojicodeInstruction vt) {
//        if (!inInstanceScope) {
//            wr().writeInstruction(stack);
//        }
//        else {
//            writeValueTypeOrObject(object, vt);
//        }
//    }
//
//    void writeValueTypeOrObject(EmojicodeInstruction object, EmojicodeInstruction vt) {
//        wr().writeInstruction(fn_->contextType() == ContextType::ValueReference ? vt : object);
//    }
//
//    void copyToVariable(CGScoper::StackIndex index, bool inInstanceScope, const Type &type) {
//        if (type.size() == 1) {
//            writeInstructionForStackOrInstance(inInstanceScope, INS_COPY_TO_STACK, INS_COPY_TO_INSTANCE_VARIABLE,
//                                               INS_COPY_VT_VARIABLE);
//            wr().writeInstruction(index.value());
//        }
//        else {
//            writeInstructionForStackOrInstance(inInstanceScope, INS_COPY_TO_STACK_SIZE,
//                                               INS_COPY_TO_INSTANCE_VARIABLE_SIZE, INS_COPY_VT_VARIABLE_SIZE);
//            wr().writeInstruction(type.size());
//            wr().writeInstruction(index.value());
//
//        }
//    }
//
//    void pushVariable(CGScoper::StackIndex index, bool inInstanceScope, const Type &type) {
//        if (type.size() == 1) {
//            writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_SINGLE_STACK, INS_PUSH_SINGLE_OBJECT,
//                                                     INS_PUSH_SINGLE_VT);
//            wr().writeInstruction(index.value());
//        }
//        else {
//            writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_WITH_SIZE_STACK, INS_PUSH_WITH_SIZE_OBJECT,
//                                                     INS_PUSH_WITH_SIZE_VT);
//            wr().writeInstruction(index.value());
//            wr().writeInstruction(type.size());
//        }
//    }
//
//    void pushVariableReference(CGScoper::StackIndex index, bool inInstanceScope) {
//        writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_VT_REFERENCE_STACK,
//                                           INS_PUSH_VT_REFERENCE_OBJECT, INS_PUSH_VT_REFERENCE_VT);
//        wr().writeInstruction(index.value());
//    }
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
