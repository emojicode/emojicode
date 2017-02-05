//
//  Thread.c
//  Emojicode
//
//  Created by Theo Weidmann on 05/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include "Thread.hpp"
#include "Processor.hpp"

Thread *Thread::lastThread_ = nullptr;
int Thread::threads_ = 0;
pthread_mutex_t threadListMutex = PTHREAD_MUTEX_INITIALIZER;

Thread* Thread::lastThread() {
    return lastThread_;
}

int Thread::threads() {
    return threads_;
}

Thread::Thread() {
#define stackSize (sizeof(StackFrame) + 10 * sizeof(Value)) * 10000
    stackLimit_ = static_cast<StackFrame *>(calloc(stackSize, 1));
    if (!stackLimit_) {
        error("Could not allocate stack!");
    }
    stackBottom_ = reinterpret_cast<StackFrame *>(reinterpret_cast<Byte *>(stackLimit_) + stackSize - 1);
    this->futureStack_ = this->stack_ = this->stackBottom_;

    pthread_mutex_lock(&threadListMutex);
    threadBefore_ = lastThread_;
    threadAfter_ = nullptr;
    if (lastThread_) {
        lastThread_->threadAfter_ = this;
    }
    lastThread_ = this;
    threads_++;
    pthread_mutex_unlock(&threadListMutex);
}

Thread::~Thread() {
    pthread_mutex_lock(&threadListMutex);
    Thread *before = this->threadBefore_;
    Thread *after = this->threadAfter_;

    if (before) before->threadAfter_ = after;
    if (after) after->threadBefore_ = before;
    else lastThread_ = before;

    threads_--;
    pthread_mutex_unlock(&threadListMutex);

    free(stackLimit_);
}

Value* Thread::reserveFrame(Value self, int size, Function *function, Value *destination,
                            EmojicodeInstruction *executionPointer) {
    StackFrame *sf = (StackFrame *)((Byte *)futureStack_ - (sizeof(StackFrame) + sizeof(Value) * size));
    if (sf < stackLimit_) {
        error("Your program triggerd a stack overflow!");
    }

    std::memset(&sf->thisContext + 1, 0, sizeof(Value) * size);

    sf->thisContext = self;
    sf->returnPointer = stack_;
    sf->returnFutureStack = futureStack_;
    sf->executionPointer = executionPointer;
    sf->destination = destination;

    futureStack_ = sf;

    return &sf->thisContext + 1;
}

void Thread::pushReservedFrame() {
    stack_ = futureStack_;
}

void Thread::pushStack(Value self, int frameSize, int argCount, Function *function, Value *destination,
                       EmojicodeInstruction *executionPointer) {
    Value *t = reserveFrame(self, frameSize, function, destination, executionPointer);

    for (int i = 0, index = 0; i < argCount; i++) {
        EmojicodeInstruction copySize = consumeInstruction();
        produce(consumeInstruction(), this, t + index);
        index += copySize;
    }

    pushReservedFrame();
}

void Thread::popStack() {
    futureStack_ = stack_->returnFutureStack;
    stack_ = stack_->returnPointer;
}

Value Thread::getVariable(int index) const {
    return *variableDestination(index);
}

Value* Thread::variableDestination(int index) const {
    return &stack_->thisContext + index + 1;
}

Object* Thread::getThisObject() const {
    return stack_->thisContext.object;
}

Value Thread::getThisContext() const {
    return stack_->thisContext;
}

EmojicodeInstruction Thread::consumeInstruction() {
    return *(stack_->executionPointer++);
}

void Thread::markStack() {
}
