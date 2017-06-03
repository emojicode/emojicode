//
//  Thread.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Thread_hpp
#define Thread_hpp

#include "Engine.hpp"
#include "RetainedObjectPointer.hpp"
#include "ThreadsManager.hpp"
#include <mutex>

namespace Emojicode {

struct StackFrame {
    StackFrame *returnPointer;
    StackFrame *returnFutureStack;
    Value *destination;
    EmojicodeInstruction *executionPointer;
    Function *function;
    unsigned int argPushIndex;

    Value thisContext;  // This must always be the very last field!

    Value* variableDestination(int index) { return &thisContext + index + 1; }
};

class Thread {
public:
    friend void gc(std::unique_lock<std::mutex> &, size_t);
    friend Thread* ThreadsManager::allocateThread();
    friend void ThreadsManager::deallocateThread(Thread *thread);
    friend Thread* ThreadsManager::nextThread(Thread *thread);

    /// Pops the stack associated with this thread
    void popStack();
    /// Pushes a new stack frame
    void pushStack(Value self, int frameSize, int argCount, Function *function, Value *destination,
                   EmojicodeInstruction *executionPointer);
    /// Pushes the reserved stack frame onto the stack
    void pushReservedFrame();

    /// Reserves a new stack frame which can later be pushed with @c stackPushReservedFrame
    /// @returns A pointer to the memory reserved for the variables.
    StackFrame* reserveFrame(Value self, int size, Function *function, Value *destination,
                             EmojicodeInstruction *executionPointer);

    StackFrame* currentStackFrame() const { return stack_; }

    /// Returns the content of the variable slot at the specific index from the stack associated with this thread
    Value variable(int index) const { return *variableDestination(index); }
    /// Returns a pointer to the variable slot at the specific index from the stack associated with this thread
    Value* variableDestination(int index) const { return stack_->variableDestination(index); }

    /// Returns the value on which the method was called.
    Value thisContext() const { return stack_->thisContext; }
    /** Returns the object on which the method was called. */
    Object* thisObject() const { return stack_->thisContext.object; }

    /// Consumes the next instruction from the current stack frame’s execution pointer, i.e. returns the value to which
    /// the pointer currently points and increments the pointer.
    EmojicodeInstruction consumeInstruction() { return *(stack_->executionPointer++); }

    /// Leaves the function currently executed. Effectively sets the execution pointer of
    /// the current stack frame to the null pointer.
    void returnFromFunction() { stack_->executionPointer = nullptr; }
    /// Leaves the function and sets the value of the return destination to the given value.
    void returnFromFunction(Value value) {
        *stack_->destination = value;
        returnFromFunction();
    }
    /// Leaves the function and sets the value of the return destination to Nothingness. (See @c makeNothingness())
    void returnNothingnessFromFunction() {
        stack_->destination->makeNothingness();
        returnFromFunction();
    }
    /// Leaves the function and sets the value of the return destination to the given value. The destination is treated
    /// as optional. (See @c optionalSet())
    void returnOEValueFromFunction(Value value) {
        stack_->destination->optionalSet(value);
        returnFromFunction();
    }
    /// Leaves the function and sets the value of the return destination to an error with the given value.
    void returnErrorFromFunction(EmojicodeInteger error) {
        stack_->destination->storeError(error);
        returnFromFunction();
    }

    /// Retains the given object.
    /// This method is used in native function code to "retain" an object, i.e. to prevent the object from being
    /// deallocated by the garbage collector and to keep a pointer to it.
    /// @warning You must retain all objects which you wish to keep, before you perform a potentially garbage-collector
    /// invoking operation. As well, you must "release" the object before your function returns by calling @c release().
    /// @returns A @c RetainedObjectPointer pointing to the retained object.
    RetainedObjectPointer retain(Object *object) {
        *retainPointer = object;
        return RetainedObjectPointer(retainPointer++);
    }
    /// Release @c n objects previously retained by @c retain().
    /// @warning Releasing more objects than you retained leads to undefined behavior.
    void release(int n) { retainPointer -= n; }
    /// Returns the this object as a retained object pointer. This method is only provided to allow convenient passing
    /// of the this object as retained object pointer. The this object itself is, of course, retained already in the
    /// stack frame and calls to @c thisObject() will always return a valid object pointer.
    /// @attention Unlike after a call to @c retain() you must not call @c release() for a call to this method.
    RetainedObjectPointer thisObjectAsRetained() const {
        return RetainedObjectPointer(&stack_->thisContext.object);
    }
    /// Returns the object pointer in the given variable slot as retained object pointer. This method is only provided
    /// to allow convenient passing of the object pointer as retained object pointer as variables are naturally always
    /// retained.
    /// @attention Unlike after a call to @c retain() you must not call @c release() for a call to this method.
    RetainedObjectPointer variableObjectPointerAsRetained(int index) const {
        return RetainedObjectPointer(&variableDestination(index)->object);
    }
private:
    Thread();
    ~Thread();

    void markStack();
    void markRetainList() const {
        for (Object **pointer = retainPointer - 1; pointer >= retainList; pointer--) mark(pointer);
    }

    StackFrame *stackLimit_;
    StackFrame *stackBottom_;
    StackFrame *stack_;
    StackFrame *futureStack_;

    Thread *threadBefore_;
    Thread *threadAfter_;

    Object *retainList[100];
    Object **retainPointer = retainList;
};

}

#endif /* Thread_hpp */
