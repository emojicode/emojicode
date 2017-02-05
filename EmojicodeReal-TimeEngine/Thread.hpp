//
//  Thread.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Thread_hpp
#define Thread_hpp

#include "Emojicode.hpp"

class Thread;

struct StackFrame {
    StackFrame *returnPointer;
    StackFrame *returnFutureStack;
    Value *destination;
    EmojicodeInstruction *executionPointer;
    Function *function;
    Value thisContext;
};

class Thread {
public:
    static Thread* lastThread();
    /// The number of threads currently active
    static int threads();

    /// Creates a new thread
    Thread();
    /// Pops the stack associated with this thread
    void popStack();
    /// Pushes a new stack frame
    void pushStack(Value self, int frameSize, int argCount, Function *function, Value *destination,
                   EmojicodeInstruction *executionPointer);
    /// Reserves a new stack frame which can later be pushed with @c stackPushReservedFrame
    /// @returns A pointer to the memory reserved for the variables.
    Value* reserveFrame(Value self, int size, Function *function, Value *destination,
                        EmojicodeInstruction *executionPointer);
    /// Pushes the reserved stack frame onto the stack
    void pushReservedFrame();

    /** Returns the value on which the method was called. */
    Value getThisContext() const;
    /** Returns the object on which the method was called. */
    Object* getThisObject() const;

    /// Gets the content of the variable at the specific index from the stack associated with this thread
    Value getVariable(int index) const;
    /// Gets a pointer to the variable at the specific index from the stack associated with this thread
    Value* variableDestination(int index) const;

    /// Consumes the next instruction from the current stack frame’s execution pointer, i.e. returns the value to which
    /// the pointer currently points and increments the pointer.
    EmojicodeInstruction consumeInstruction();

    void markStack();

    StackFrame* currentStackFrame() const { return stack_; }

    Thread* threadBefore() const { return threadBefore_; }

    Object* const& retain(Object *object) { *retainPointer = object; return *(retainPointer++); }
    void release(int n) { retainPointer -= n; }

    ~Thread();
private:
    static Thread *lastThread_;
    static int threads_;

    StackFrame *stackLimit_;
    StackFrame *stackBottom_;
    StackFrame *stack_;
    StackFrame *futureStack_;

    Thread *threadBefore_;
    Thread *threadAfter_;

    Object *retainList[100];
    Object **retainPointer = retainList;
};

#endif /* Thread_hpp */
