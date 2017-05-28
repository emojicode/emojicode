//
//  Thread.c
//  Emojicode
//
//  Created by Theo Weidmann on 05/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Thread.hpp"
#include "Object.hpp"
#include "Processor.hpp"
#include <cstdlib>
#include <cstring>
#include <thread>

using namespace Emojicode;

Thread::Thread() {
#define stackSize ((sizeof(StackFrame) + 10 * sizeof(Value)) * 10000)
    stackLimit_ = static_cast<StackFrame *>(calloc(stackSize, 1));
    if (!stackLimit_) {
        error("Could not allocate stack!");
    }
    stackBottom_ = reinterpret_cast<StackFrame *>(reinterpret_cast<Byte *>(stackLimit_) + stackSize - 1);
    this->futureStack_ = this->stack_ = this->stackBottom_;
}

Thread::~Thread() {
    free(stackLimit_);
}

StackFrame* Thread::reserveFrame(Value self, int size, Function *function, Value *destination,
                            EmojicodeInstruction *executionPointer) {
    auto *sf = (StackFrame *)((Byte *)futureStack_ - (sizeof(StackFrame) + sizeof(Value) * size));
    if (sf < stackLimit_) {
        error("Your program triggerd a stack overflow!");
    }

    sf->thisContext = self;
    sf->returnPointer = stack_;
    sf->returnFutureStack = futureStack_;
    sf->executionPointer = executionPointer;
    sf->destination = destination;
    sf->function = function;

    futureStack_ = sf;

    return sf;
}

void Thread::pushReservedFrame() {
    futureStack_->argPushIndex = UINT32_MAX;
    stack_ = futureStack_;
}

void Thread::pushStack(Value self, int frameSize, int argCount, Function *function, Value *destination,
                       EmojicodeInstruction *executionPointer) {
    StackFrame *sf = reserveFrame(self, frameSize, function, destination, executionPointer);

    sf->argPushIndex = 0;

    for (int i = 0; i < argCount; i++) {
        EmojicodeInstruction copySize = consumeInstruction();
        produce(this, sf->variableDestination(0) + sf->argPushIndex);
        sf->argPushIndex += copySize;
    }

    pushReservedFrame();
}

void Thread::popStack() {
    futureStack_ = stack_->returnFutureStack;
    stack_ = stack_->returnPointer;
}

void Thread::markStack() {
    for (auto frame = futureStack_; frame < stackBottom_; frame = frame->returnFutureStack) {
        unsigned int delta = frame->executionPointer ? frame->executionPointer - frame->function->block.instructions : 0;
        for (unsigned int i = 0; i < frame->function->objectVariableRecordsCount; i++) {
            auto record = frame->function->objectVariableRecords[i];
            if (record.from <= delta && delta <= frame->function->objectVariableRecords[i].to
                && record.variableIndex < frame->argPushIndex) {
                markByObjectVariableRecord(record, frame->variableDestination(0), i);
            }
        }
    }
}
