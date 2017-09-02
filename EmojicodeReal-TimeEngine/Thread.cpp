//
//  Thread.c
//  Emojicode
//
//  Created by Theo Weidmann on 05/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Thread.hpp"
#include "Memory.hpp"
#include "Processor.hpp"
#include <cstdlib>
#include <cstring>
#include <thread>

using namespace Emojicode;

Thread::Thread() {
#define stackSize (sizeof(StackFrame) * 1000000)
    stackLimit_ = static_cast<StackFrame *>(calloc(stackSize, 1));
    if (!stackLimit_) {
        error("Could not allocate stack!");
    }
    stackBottom_ = reinterpret_cast<StackFrame *>(reinterpret_cast<Byte *>(stackLimit_) + stackSize);
    this->stack_ = this->stackBottom_;
}

Thread::~Thread() {
    free(stackLimit_);
}

void Thread::debugOprStack() {
    if (rstackPointer_ < rstack_) {
        puts("Stack underflow!");
        return;
    }
    puts("                           ☚ Stack Pointer");
    for (auto p = rstackPointer_ - 1; p >= rstack_; p--) {
        puts(  "╔═════════════════════════╗");
        printf("║%12lld "            "%12p║\n", p->raw, p->object);
        puts(  "╚═════════════════════════╝");
    }
    puts("┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅");
}

StackFrame* Thread::pushStackFrame(Value self, bool copyArgs, Function *function) {
    size_t fullSize = sizeof(StackFrame) + sizeof(Value) * function->frameSize;

    auto *sf = (StackFrame *)((Byte *)stack_ - (fullSize + (fullSize % alignof(StackFrame))));
    if (sf < stackLimit_) {
        error("Your program triggerd a stack overflow!");
    }

    sf->thisContext = self;
    sf->returnPointer = stack_;
    sf->executionPointer = function->block.instructions;
    sf->function = function;

    if (copyArgs) {
        EmojicodeInstruction copySize = consumeInstruction();
        std::memcpy(sf->variableDestination(0), popOpr(copySize), copySize * sizeof(Value));
    }
#ifdef DEBUG
    puts("=== PUSH FRAME ===");
#endif
    stack_ = sf;
    return stack_;
}

void Thread::popStackFrame() {
#ifdef DEBUG
    puts("=== POP FRAME ===");
#endif
    stack_ = stack_->returnPointer;
}

void Thread::markStack() {
    for (auto frame = stack_; frame < stackBottom_; frame = frame->returnPointer) {
        unsigned int delta = frame->executionPointer ? frame->executionPointer - frame->function->block.instructions : 0;
        switch (frame->function->context) {
            case ContextType::Object:
                mark(&frame->thisContext.object);
                break;
            case ContextType::ValueReference:
                markValueReference(&frame->thisContext.value);
                break;
            default:
                break;
        }
        for (unsigned int i = 0; i < frame->function->objectVariableRecordsCount; i++) {
            auto record = frame->function->objectVariableRecords[i];
            if (record.from <= delta && delta <= record.to) {
                markByObjectVariableRecord(record, frame->variableDestination(0), i);
            }
        }
    }
}
