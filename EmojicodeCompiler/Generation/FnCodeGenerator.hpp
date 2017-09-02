//
//  FnCodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 29/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef FnCodeGenerator_hpp
#define FnCodeGenerator_hpp

#include "../../EmojicodeInstructions.h"
#include "../AST/ASTNode.hpp"
#include "../Functions/Function.hpp"
#include "../Scoping/CGScoper.hpp"
#include "FunctionWriter.hpp"
#include <memory>

namespace EmojicodeCompiler {

class FnCodeGenerator {
public:
    explicit FnCodeGenerator(Function *function)
    : fn_(function), scoper_(function->variableCount()),
    instanceScoper_(function->owningType().type() != TypeType::NoReturn ?
                    &function->owningType().typeDefinition()->cgScoper() : nullptr) {}
    void generate();

    FunctionWriter& wr() { return fn_->writer_; }
    CGScoper& scoper() { return scoper_; }
    CGScoper& instanceScoper() { return *instanceScoper_; }
    Application* app() { return fn_->package()->app(); }

    void writeInteger(long long value);

    void writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                            EmojicodeInstruction object, EmojicodeInstruction vt) {
        if (!inInstanceScope) {
            wr().writeInstruction(stack);
        }
        else {
            writeValueTypeOrObject(object, vt);
        }
    }

    void writeValueTypeOrObject(EmojicodeInstruction object, EmojicodeInstruction vt) {
        wr().writeInstruction(fn_->contextType() == ContextType::ValueReference ? vt : object);
    }

    void copyToVariable(CGScoper::StackIndex index, bool inInstanceScope, const Type &type) {
        if (type.size() == 1) {
            writeInstructionForStackOrInstance(inInstanceScope, INS_COPY_TO_STACK, INS_COPY_TO_INSTANCE_VARIABLE,
                                               INS_COPY_VT_VARIABLE);
            wr().writeInstruction(index.value());
        }
        else {
            writeInstructionForStackOrInstance(inInstanceScope, INS_COPY_TO_STACK_SIZE,
                                               INS_COPY_TO_INSTANCE_VARIABLE_SIZE, INS_COPY_VT_VARIABLE_SIZE);
            wr().writeInstruction(type.size());
            wr().writeInstruction(index.value());

        }
    }

    void pushVariable(CGScoper::StackIndex index, bool inInstanceScope, const Type &type) {
        if (type.size() == 1) {
            writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_SINGLE_STACK, INS_PUSH_SINGLE_OBJECT,
                                                     INS_PUSH_SINGLE_VT);
            wr().writeInstruction(index.value());
        }
        else {
            writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_WITH_SIZE_STACK, INS_PUSH_WITH_SIZE_OBJECT,
                                                     INS_PUSH_WITH_SIZE_VT);
            wr().writeInstruction(index.value());
            wr().writeInstruction(type.size());
        }
    }

    void pushVariableReference(CGScoper::StackIndex index, bool inInstanceScope) {
        writeInstructionForStackOrInstance(inInstanceScope, INS_PUSH_VT_REFERENCE_STACK,
                                           INS_PUSH_VT_REFERENCE_OBJECT, INS_PUSH_VT_REFERENCE_VT);
        wr().writeInstruction(index.value());
    }
protected:
    virtual void declareArguments();
private:
    Function *fn_;
    CGScoper scoper_;
    CGScoper *instanceScoper_;

    void generateBoxingLayer();
};

}  // namespace EmojicodeCompiler

#endif /* FnCodeGenerator_hpp */
