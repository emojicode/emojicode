//
//  Stack.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.08.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Emojicode.h"
#include <string.h>

Something* stackReserveFrame(Something this, uint8_t variableCount, Thread *thread){
    StackFrame *sf = (StackFrame *)(thread->futureStack - (sizeof(StackFrame) + sizeof(Something) * variableCount));
    if ((Byte *)sf < thread->stackLimit) {
        error("Your program triggerd a stack overflow!");
    }
    
    memset((Byte *)sf + sizeof(StackFrame), 0, sizeof(Something) * variableCount);
    
    sf->thisContext = this;
    sf->variableCount = variableCount;
    sf->returnPointer = thread->stack;
    sf->returnFutureStack = thread->futureStack;
    
    thread->futureStack = (Byte *)sf;
    
    return (Something *)(((Byte *)sf) + sizeof(StackFrame));
}

void stackPushReservedFrame(Thread *thread){
    thread->stack = thread->futureStack;
}

void stackPush(Something this, uint8_t variableCount, uint8_t argCount, Thread *thread){
    Something *t = stackReserveFrame(this, variableCount, thread);
    
    for (uint8_t i = 0; i < argCount; i++) {
        t[i] = parse(consumeCoin(thread), thread);
    }
    
    stackPushReservedFrame(thread);
}

void stackPop(Thread *thread) {
    thread->futureStack = ((StackFrame *)thread->stack)->returnFutureStack;
    thread->stack = ((StackFrame *)thread->stack)->returnPointer;
}

StackState storeStackState(Thread *thread) {
    StackState s = {thread->futureStack, thread->stack};
    return s;
}

void restoreStackState(StackState s, Thread *thread) {
    thread->futureStack = s.futureStack;
    thread->stack = s.stack;
}

Something stackGetVariable(uint8_t index, Thread *thread){
    return *(Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index);
}

void stackDecrementVariable(uint8_t index, Thread *thread){
    ((Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index))->raw--;
}

void stackIncrementVariable(uint8_t index, Thread *thread){
    ((Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index))->raw++;
}

void stackSetVariable(uint8_t index, Something value, Thread *thread){
    *(Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index) = value;
}

Object* stackGetThisObject(Thread *thread){
    return ((StackFrame *)thread->stack)->thisContext.object;
}

Something stackGetThisContext(Thread *thread){
    return ((StackFrame *)thread->stack)->thisContext;
}

Class* stackGetThisObjectClass(Thread *thread){
    return ((StackFrame *)thread->stack)->thisContext.eclass;
}

void stackMark(Thread *thread){
    for (StackFrame *stackFrame = (StackFrame *)thread->futureStack; (Byte *)stackFrame < thread->stackBottom; stackFrame = stackFrame->returnFutureStack) {
        for (uint8_t i = 0; i < stackFrame->variableCount; i++) {
            Something *s = (Something *)(((Byte *)stackFrame) + sizeof(StackFrame) + sizeof(Something) * i);
            if (isRealObject(*s)) {
                mark(&s->object);
            }
        }
        if (isRealObject(stackFrame->thisContext)) {
            mark(&stackFrame->thisContext.object);
        }
    }
}