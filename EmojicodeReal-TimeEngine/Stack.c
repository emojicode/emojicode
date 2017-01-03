//
//  Stack.c
//  Emojicode
//
//  Created by Theo Weidmann on 02.08.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Emojicode.h"
#include <string.h>

Something* stackReserveFrame(Something this, int size, Thread *thread) {
    StackFrame *sf = (StackFrame *)(thread->futureStack - (sizeof(StackFrame) + sizeof(Something) * size));
    if ((Byte *)sf < thread->stackLimit) {
        error("Your program triggerd a stack overflow!");
    }
    
    sf->thisContext = this;
    sf->returnPointer = thread->stack;
    sf->returnFutureStack = thread->futureStack;
    
    thread->futureStack = (Byte *)sf;
    
    return (Something *)(((Byte *)sf) + sizeof(StackFrame));
}

void stackPushReservedFrame(Thread *thread){
    thread->stack = thread->futureStack;
}

void stackPush(Something this, int frameSize, uint8_t argCount, Thread *thread) {
    Something *t = stackReserveFrame(this, frameSize, thread);
    
    for (int i = 0, index = 0; i < argCount; i++) {
        EmojicodeCoin copySize = consumeCoin(thread);
        produce(consumeCoin(thread), thread, t + index);
        index += copySize;
    }
    
    stackPushReservedFrame(thread);
}

void stackPop(Thread *thread) {
    StackFrame *frame = ((StackFrame *)thread->stack);
    thread->futureStack = frame->returnFutureStack;
    thread->stack = frame->returnPointer;
    // Avoid randomly leaving behind an object reference as returnPointer takes the place of Type
    frame->returnPointer = (void *)T_INTEGER;
}

StackState storeStackState(Thread *thread) {
    StackState s = {thread->futureStack, thread->stack};
    return s;
}

void restoreStackState(StackState s, Thread *thread) {
    thread->futureStack = s.futureStack;
    thread->stack = s.stack;
}

Something stackGetVariable(int index, Thread *thread) {
    return *stackVariableDestination(index, thread);
}

Something* stackVariableDestination(int index, Thread *thread) {
    return (Something *)(thread->stack + sizeof(StackFrame) + sizeof(Something) * index);
}

Object* stackGetThisObject(Thread *thread) {
    return ((StackFrame *)thread->stack)->thisContext.object;
}

Something stackGetThisContext(Thread *thread) {
    return ((StackFrame *)thread->stack)->thisContext;
}

void stackMark(Thread *thread) {
    Something *nextStackFrame = (Something *)thread->futureStack;
    for (Something *something = nextStackFrame; (Byte *)something < thread->stackBottom; something++) {
        if (isRealObject(*something)) {
            mark(&something->object);
        }
        if (nextStackFrame == something) {  // Check here, because first StackFrame member is Something.
            nextStackFrame = (Something *)((StackFrame *)something)->returnFutureStack;
            something++;  // Jump over the return pointers, which have been aligned as Something
        }
    }
}
