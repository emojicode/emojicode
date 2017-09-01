//
//  Processor.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Processor.hpp"
#include "../EmojicodeInstructions.h"
#include "Class.hpp"
#include "Dictionary.hpp"
#include "List.hpp"
#include "String.hpp"
#include "Thread.hpp"
#include <cmath>
#include <cstring>
#include <functional>
#include <thread>

namespace Emojicode {

inline double readDouble(Thread *thread) {
    EmojicodeInteger scale = (static_cast<EmojicodeInteger>(thread->consumeInstruction()) << 32) ^ thread->consumeInstruction();
    EmojicodeInteger exp = thread->consumeInstruction();

    return ldexp(static_cast<double>(scale)/PORTABLE_INTLEAST64_MAX, static_cast<int>(exp));
}

void loadCapture(Closure *c, Thread *thread) {
    if (c->captureSize == 0) {
        return;
    }
    auto *cv = c->capturedVariables->val<Value>();
    std::memcpy(thread->variableDestination(c->captureDestination), cv, c->captureSize * sizeof(Value));
}

void executeCallableExtern(Object *callable, Value *args, size_t argsSize, Thread *thread) {
    auto *c = callable->val<Closure>();
    auto sf = thread->pushStackFrame(c->thisContext, false, c->function);
    std::memcpy(sf->variableDestination(0), args, argsSize * sizeof(Value));
    loadCapture(c, thread);
    auto interupt = thread->configureInterruption();
    execute(thread);
    thread->deconfigureInterruption(interupt);
}

inline EmojicodeInteger normalizedBoxType(EmojicodeInteger type) {
    return type & ~REMOTE_MASK;
}

void execute(Thread *thread) {
    while (true) {
        auto i = static_cast<Instructions>(thread->consumeInstruction());
#ifdef DEBUG
        pinsname(i);
        puts("");
#endif
        switch (i) {
            case INS_DISPATCH_METHOD: {
                EmojicodeInstruction vti = thread->consumeInstruction();
                Value v = thread->popOpr();
                thread->pushStackFrame(v, true, v.object->klass->methodsVtable[vti]);
                continue;
            }
            case INS_DISPATCH_TYPE_METHOD: {
                Value v = thread->popOpr();
                EmojicodeInstruction vti = thread->consumeInstruction();
                thread->pushStackFrame(v, true, v.klass->methodsVtable[vti]);
                continue;
            }
            case INS_DISPATCH_PROTOCOL: {
                EmojicodeInstruction pti = thread->consumeInstruction();
                EmojicodeInstruction vti = thread->consumeInstruction();

                auto v = thread->popOpr();

                auto type = v.value[0].raw;
                if (type == T_OBJECT) {
                    Object *o = v.value[1].object;
                    thread->pushStackFrame(v.value[1], true, o->klass->protocolTable.dispatch(pti, vti));
                }
                else if ((type & REMOTE_MASK) != 0) {
                    auto protocol = protocolDispatchTableTable[normalizedBoxType(type) - protocolDTTOffset];
                    thread->pushStackFrame(v.value[1].object->val<Value>(), true, protocol.dispatch(pti, vti));
                }
                else {
                    thread->pushStackFrame(v.value + 1, true,
                                           protocolDispatchTableTable[type - protocolDTTOffset].dispatch(pti, vti));
                }
                continue;
            }
            case INS_NEW_OBJECT: {
                Class *klass = thread->popOpr().klass;
                Object *object = newObject(klass);
                Function *initializer = klass->initializersVtable[thread->consumeInstruction()];
                thread->pushStackFrame(object, true, initializer);
                continue;
            }
            case INS_DISPATCH_SUPER: {
                Class *klass = thread->popOpr().klass;
                EmojicodeInstruction vti = thread->consumeInstruction();
                thread->pushStackFrame(thread->thisContext(), true, klass->methodsVtable[vti]);
                continue;
            }
            case INS_CALL_CONTEXTED_FUNCTION:
                thread->pushStackFrame(thread->popOpr(), true, functionTable[thread->consumeInstruction()]);
                continue;
            case INS_CALL_FUNCTION:
                thread->pushStackFrame(Value(), true, functionTable[thread->consumeInstruction()]);
                continue;
            case INS_SIMPLE_OPTIONAL_PRODUCE: {
                thread->pushOpr(static_cast<EmojicodeInteger>(T_OPTIONAL_VALUE));
                continue;
            }
            case INS_PUSH_ERROR:
                thread->pushOpr(static_cast<EmojicodeInteger>(T_ERROR));
                continue;
            case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE: {
                auto *box = thread->popOpr(kBoxValueSize);
                EmojicodeInstruction size = thread->consumeInstruction();

                if (box[0].raw != T_NOTHINGNESS) {
                    thread->pushOpr(T_OPTIONAL_VALUE);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                }
                thread->pushPointerOpr(size);  // The values of the box are still there, the type has been overwritten
                continue;
            }
            case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE: {
                auto *box = thread->popOpr(kBoxValueSize);
                auto *value = box[1].object->val<Value>();
                EmojicodeInstruction size = thread->consumeInstruction();
                if (box[0].raw != T_NOTHINGNESS) {
                    thread->pushOpr(T_OPTIONAL_VALUE);  // We have just invalidated box!
                    thread->pushOpr(value, size);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(size);
                }
                continue;
            }
            case INS_SIMPLE_OPTIONAL_TO_BOX: {
                EmojicodeInstruction typeId = thread->consumeInstruction();
                auto size = thread->consumeInstruction();
                thread->popOpr(size);
                if (thread->popOpr().raw != T_NOTHINGNESS) {
                    thread->pushOpr(static_cast<EmojicodeInteger>(typeId));
                    thread->pushPointerOpr(size);
                    thread->pushPointerOpr(kBoxValueSize - 1 - size);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(kBoxValueSize - 1);
                }
                continue;
            }
            case INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE: {
                EmojicodeInstruction typeId = thread->consumeInstruction();
                auto size = thread->consumeInstruction();
                auto *src = thread->popOpr(size + 1);

                if (src->raw != T_NOTHINGNESS) {
                    auto *object = newArray(size * sizeof(Value));
                    std::memcpy(object->val<Value>(), src + 1, size * sizeof(Value));

                    thread->pushOpr(static_cast<EmojicodeInteger>(typeId));
                    thread->pushOpr(object);
                    thread->pushPointerOpr(kBoxValueSize - 2);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(kBoxValueSize - 1);
                }
                continue;
            }
            case INS_PUSH_N:
                thread->pushPointerOpr(thread->consumeInstruction());
                continue;
            case INS_POP:
                thread->popOpr();
                continue;
            case INS_BOX_PRODUCE:
                thread->pushOpr(static_cast<EmojicodeInteger>(thread->consumeInstruction()));
                continue;
            case INS_BOX_PRODUCE_REMOTE: {
                auto size = thread->consumeInstruction();
                auto object = newArray(size * sizeof(Value));
                std::memcpy(object->val<Value>(), thread->popOpr(size), size * sizeof(Value));
                thread->pushOpr(static_cast<EmojicodeInteger>(thread->consumeInstruction()));
                thread->pushOpr(object);
                thread->pushPointerOpr(kBoxValueSize - 2);
                continue;
            }
            case INS_UNBOX: {
                EmojicodeInstruction size = thread->consumeInstruction();
                thread->popThenPushOpr(kBoxValueSize, 1, size);
                continue;
            }
            case INS_UNBOX_REMOTE: {
                auto *value = thread->popOpr(kBoxValueSize)[1].object->val<Value>();
                EmojicodeInstruction size = thread->consumeInstruction();
                thread->pushOpr(value, size);
                continue;
            }
            case INS_PUSH_VT_REFERENCE_STACK:
                thread->pushOpr(thread->variableDestination(thread->consumeInstruction()));
                continue;
            case INS_PUSH_VT_REFERENCE_OBJECT:
                thread->pushOpr(thread->thisObject()->variableDestination(thread->consumeInstruction()));
                continue;
            case INS_PUSH_VT_REFERENCE_VT:
                thread->pushOpr(thread->thisContext().value + thread->consumeInstruction());
                continue;
            case INS_PUSH_STACK_REFERENCE_N_BACK:
                thread->pushOpr(thread->pointerOpr() - thread->consumeInstruction());
                continue;
            case INS_GET_CLASS_FROM_INSTANCE:
                thread->pushOpr(thread->popOpr().object->klass);
                continue;
            case INS_GET_CLASS_FROM_INDEX:
                thread->pushOpr(classTable[thread->consumeInstruction()]);
                continue;
            case INS_GET_STRING_POOL:
                thread->pushOpr(stringPool[thread->consumeInstruction()]);
                continue;
            case INS_GET_TRUE:
                thread->pushOpr(true);
                continue;
            case INS_GET_FALSE:
                thread->pushOpr(false);
                continue;
            case INS_GET_32_INTEGER:
                thread->pushOpr(static_cast<EmojicodeInteger>(thread->consumeInstruction()) - INT32_MAX);
                continue;
            case INS_GET_64_INTEGER: {
                EmojicodeInteger a = thread->consumeInstruction();
                thread->pushOpr(a << 32 | thread->consumeInstruction());
                continue;
            }
            case INS_GET_DOUBLE:
                thread->pushOpr(readDouble(thread));
                continue;
            case INS_GET_SYMBOL:
                thread->pushOpr(static_cast<EmojicodeChar>(thread->consumeInstruction()));
                continue;
            case INS_GET_NOTHINGNESS:
                thread->pushOpr(T_NOTHINGNESS);
                continue;
            case INS_COPY_TO_STACK:
                *thread->variableDestination(thread->consumeInstruction()) = thread->popOpr();
                continue;
            case INS_COPY_TO_INSTANCE_VARIABLE:
                *thread->thisObject()->variableDestination(thread->consumeInstruction()) = thread->popOpr();
                continue;
            case INS_COPY_VT_VARIABLE:
                *(thread->thisContext().value + thread->consumeInstruction()) = thread->popOpr();
                continue;
            case INS_COPY_TO_STACK_SIZE: {
                auto n = thread->consumeInstruction();
                std::memcpy(thread->variableDestination(thread->consumeInstruction()),
                            thread->popOpr(n), n * sizeof(Value));
                continue;
            }
            case INS_COPY_TO_INSTANCE_VARIABLE_SIZE: {
                auto n = thread->consumeInstruction();
                std::memcpy(thread->thisObject()->variableDestination(thread->consumeInstruction()),
                            thread->popOpr(n), n * sizeof(Value));
                continue;
            }
            case INS_COPY_VT_VARIABLE_SIZE: {
                auto n = thread->consumeInstruction();
                std::memcpy(thread->thisContext().value + thread->consumeInstruction(),
                            thread->popOpr(n), n * sizeof(Value));
                continue;
            }
            case INS_PUSH_SINGLE_STACK:
                thread->pushOpr(thread->variable(thread->consumeInstruction()));
                continue;
            case INS_PUSH_WITH_SIZE_STACK: {
                auto *v = thread->variableDestination(thread->consumeInstruction());
                thread->pushOpr(v, thread->consumeInstruction());
                continue;
            }
            case INS_PUSH_SINGLE_OBJECT:
                thread->pushOpr(*thread->thisObject()->variableDestination(thread->consumeInstruction()));
                continue;
            case INS_PUSH_WITH_SIZE_OBJECT: {
                Value *source = thread->thisObject()->variableDestination(thread->consumeInstruction());
                thread->pushOpr(source, thread->consumeInstruction());
                continue;
            }
            case INS_PUSH_SINGLE_VT:
                thread->pushOpr(thread->thisContext().value[thread->consumeInstruction()]);
                continue;
            case INS_PUSH_WITH_SIZE_VT: {
                Value *source = thread->thisContext().value + thread->consumeInstruction();
                thread->pushOpr(source, thread->consumeInstruction());
                continue;
            }
            case INS_PUSH_VALUE_FROM_REFERENCE:
                thread->pushOpr(thread->popOpr().value, thread->consumeInstruction());
                continue;
            // Operators
            case INS_EQUAL_PRIMITIVE:
                thread->pushOpr(thread->popOpr().raw == thread->popOpr().raw);
                continue;
            case INS_EQUAL_SYMBOL:
                thread->pushOpr(thread->popOpr().character == thread->popOpr().character);
                continue;
            case INS_SUBTRACT_INTEGER: {
                auto b = thread->popOpr().raw;
                thread->pushOpr(thread->popOpr().raw - b);
                continue;
            }
            case INS_ADD_INTEGER:
                thread->pushOpr(thread->popOpr().raw + thread->popOpr().raw);
                continue;
            case INS_MULTIPLY_INTEGER:
                thread->pushOpr(thread->popOpr().raw * thread->popOpr().raw);
                continue;
            case INS_DIVIDE_INTEGER: {
                auto b = thread->popOpr().raw;
                thread->pushOpr(thread->popOpr().raw / b);
                continue;
            }
            case INS_REMAINDER_INTEGER: {
                auto b = thread->popOpr().raw;
                thread->pushOpr(thread->popOpr().raw % b);
                continue;
            }
            case INS_INVERT_BOOLEAN:
                thread->pushOpr(!thread->popOpr().raw);
                continue;
            case INS_OR_BOOLEAN:
                thread->pushOpr(thread->popOpr().raw || thread->popOpr().raw);
                continue;
            case INS_AND_BOOLEAN:
                thread->pushOpr(thread->popOpr().raw && thread->popOpr().raw);
                continue;
            case INS_GREATER_INTEGER: {
                auto b = thread->popOpr().raw;
                thread->pushOpr(thread->popOpr().raw > b);
                continue;
            }
            case INS_GREATER_OR_EQUAL_INTEGER: {
                auto b = thread->popOpr().raw;
                thread->pushOpr(thread->popOpr().raw >= b);
                continue;
            }
            case INS_SAME_OBJECT:
                thread->pushOpr(thread->popOpr().object == thread->popOpr().object);
                continue;
            case INS_IS_NOTHINGNESS:
                thread->pushOpr(thread->popOpr().value->raw == T_NOTHINGNESS);
                continue;
            case INS_IS_ERROR:
                thread->pushOpr(thread->popOpr().value->raw == T_ERROR);
                continue;
            case INS_EQUAL_DOUBLE:
                thread->pushOpr(thread->popOpr().doubl == thread->popOpr().doubl);
                continue;
            case INS_SUBTRACT_DOUBLE: {
                auto b = thread->popOpr().doubl;
                thread->pushOpr(thread->popOpr().doubl - b);
                continue;
            }
            case INS_ADD_DOUBLE:
                thread->pushOpr(thread->popOpr().doubl + thread->popOpr().doubl);
                continue;
            case INS_MULTIPLY_DOUBLE:
                thread->pushOpr(thread->popOpr().doubl * thread->popOpr().doubl);
                continue;
            case INS_DIVIDE_DOUBLE: {
                auto b = thread->popOpr().doubl;
                thread->pushOpr(thread->popOpr().doubl / b);
                continue;
            }
            case INS_GREATER_DOUBLE: {
                auto b = thread->popOpr().doubl;
                thread->pushOpr(thread->popOpr().doubl > b);
                continue;
            }
            case INS_GREATER_OR_EQUAL_DOUBLE: {
                auto b = thread->popOpr().doubl;
                thread->pushOpr(thread->popOpr().doubl >= b);
                continue;
            }
            case INS_REMAINDER_DOUBLE: {
                auto b = thread->popOpr().doubl;
                thread->pushOpr(fmod(thread->popOpr().doubl, b));
                continue;
            }
            case INS_INT_TO_DOUBLE:
                thread->pushOpr(static_cast<double>(thread->popOpr().raw));
                continue;
            case INS_UNWRAP_SIMPLE_OPTIONAL: {
                EmojicodeInstruction n = thread->consumeInstruction();
                auto *v = thread->popOpr().value;
                if (v->raw != T_NOTHINGNESS) {
                    thread->pushOpr(v + 1, n);
                }
                else {
                    error("Unexpectedly found ✨ while unwrapping a 🍬.");
                }
                continue;
            }
            case INS_UNWRAP_BOX_OPTIONAL: {
                auto *box = thread->popOpr().value;
                if (box->raw != T_NOTHINGNESS) {
                    thread->pushOpr(box, kBoxValueSize);
                }
                else {
                    error("Unexpectedly found ✨ while unwrapping a 🍬.");
                }
                continue;
            }
            case INS_ERROR_CHECK_SIMPLE_OPTIONAL: {
                EmojicodeInteger n = thread->consumeInstruction();
                auto *v = thread->popOpr().value;
                if (v->raw != T_ERROR) {
                    thread->pushOpr(v + 1, n);
                }
                else {
                    error("Unexpectedly found 🚨 with value %d.", v[1].raw);
                }
                continue;
            }
            case INS_ERROR_CHECK_BOX_OPTIONAL: {
                auto *box = thread->popOpr().value;
                if (box->raw != T_ERROR) {
                    thread->pushOpr(box, kBoxValueSize);
                }
                else {
                    error("Unexpectedly found 🚨 with value %d.", box[1].raw);
                }
                continue;
            }
            case INS_THIS:
                thread->pushOpr(thread->thisContext());
                continue;
            case INS_SUPER_INITIALIZER: {
                Class *klass = thread->popOpr().klass;

                EmojicodeInstruction vti = thread->consumeInstruction();
                Function *initializer = klass->initializersVtable[vti];

                thread->pushStackFrame(thread->thisContext(), true, initializer);
                continue;
            }
            case INS_DOWNCAST_TO_CLASS: {
                auto v = thread->popOpr();
                Class *klass = thread->popOpr().klass;
                if (v.object->klass->inheritsFrom(klass)) {
                    thread->pushOpr(T_OPTIONAL_VALUE);
                    thread->pushOpr(v);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(1);
                }
                continue;
            }
            case INS_CAST_TO_CLASS: {
                auto *box = thread->popOpr(kBoxValueSize);
                Class *klass = thread->popOpr().klass;
                if (box[0].raw == T_OBJECT && box[1].object->klass->inheritsFrom(klass)) {
                    auto o = box[1];
                    thread->pushOpr(T_OPTIONAL_VALUE);
                    thread->pushOpr(o);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(1);
                }
                continue;
            }
            case INS_CAST_TO_PROTOCOL: {
                auto *box = thread->popOpr(kBoxValueSize);
                EmojicodeInstruction pi = thread->consumeInstruction();
                auto typeId = normalizedBoxType(box[0].raw) - protocolDTTOffset;
                if (box[0].raw != T_NOTHINGNESS && ((box[0].raw == T_OBJECT &&
                                                     box[1].object->klass->protocolTable.conformsTo(pi)) ||
                                                    protocolDispatchTableTable[typeId].conformsTo(pi))) {
                    thread->pushPointerOpr(kBoxValueSize);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(kBoxValueSize - 1);
                }
                continue;
            }
            case INS_CAST_TO_VALUE_TYPE: {
                auto *box = thread->popOpr(kBoxValueSize);
                EmojicodeInstruction id = thread->consumeInstruction();
                if (box[0].raw == id) {
                    thread->pushPointerOpr(kBoxValueSize);
                }
                else {
                    thread->pushOpr(T_NOTHINGNESS);
                    thread->pushPointerOpr(kBoxValueSize - 1);
                }
                continue;
            }
            case INS_BINARY_AND_INTEGER:
                thread->pushOpr(thread->popOpr().raw & thread->popOpr().raw);
                continue;
            case INS_BINARY_OR_INTEGER:
                thread->pushOpr(thread->popOpr().raw | thread->popOpr().raw);
                continue;
            case INS_BINARY_XOR_INTEGER:
                thread->pushOpr(thread->popOpr().raw ^ thread->popOpr().raw);
                continue;
            case INS_BINARY_NOT_INTEGER:
                thread->pushOpr(~thread->popOpr().raw);
                continue;
            case INS_SHIFT_LEFT_INTEGER:
                thread->pushOpr(thread->popOpr().raw << thread->popOpr().raw);
                continue;
            case INS_SHIFT_RIGHT_INTEGER:
                thread->pushOpr(thread->popOpr().raw >> thread->popOpr().raw);
                continue;
            case INS_RETURN:
                if (thread->interrupt()) {
                    return;
                }
                thread->returnFromFunction();
                continue;
            case INS_JUMP_FORWARD:
                thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
                continue;
            case INS_JUMP_FORWARD_IF:
                if (thread->popOpr().raw) {
                    thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
                }
                else {
                    thread->consumeInstruction();
                }
                continue;
            case INS_JUMP_BACKWARD_IF:
                if (thread->popOpr().raw) {
                    auto a = thread->consumeInstruction();
                    thread->currentStackFrame()->executionPointer -= a;
                }
                else {
                    thread->consumeInstruction();
                }
                continue;
            case INS_JUMP_FORWARD_IF_NOT:
                if (!thread->popOpr().raw) {
                    thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
                }
                else {
                    thread->consumeInstruction();
                }
                continue;
            case INS_JUMP_BACKWARD_IF_NOT:
                if (!thread->popOpr().raw) {
                    thread->currentStackFrame()->executionPointer -= thread->consumeInstruction();
                }
                else {
                    thread->consumeInstruction();
                }
                continue;
            case INS_TRANSFER_CONTROL_TO_NATIVE:
                thread->currentStackFrame()->function->handler(thread);
                continue;
            case INS_EXECUTE_CALLABLE: {
                auto *c = thread->popOpr().object->val<Closure>();
                thread->pushStackFrame(c->thisContext, true, c->function);
                loadCapture(c, thread);
                continue;
            }
            case INS_CLOSURE: {
                auto closure = thread->retain(newObject(CL_CLOSURE));

                auto *c = closure->val<Closure>();

                c->function = functionTable[thread->consumeInstruction()];
                auto count = thread->consumeInstruction();
                c->captureSize = thread->consumeInstruction();
                c->captureDestination = thread->consumeInstruction();

                Object *captures = newArray(sizeof(Value) * c->captureSize);
                c = closure->val<Closure>();
                c->capturedVariables = captures;

                auto *t = c->capturedVariables->val<Value>();
                for (unsigned int i = 0; i < count; i++) {
                    EmojicodeInstruction index = thread->consumeInstruction();
                    EmojicodeInstruction size = thread->consumeInstruction();
                    std::memcpy(t, thread->variableDestination(index), size * sizeof(Value));
                    t += size;
                }

                auto recordCount = thread->consumeInstruction();
                c->recordsCount = recordCount;
                Object *objectVariableRecordsObject = newArray(sizeof(ObjectVariableRecord) * recordCount);
                closure->val<Closure>()->objectVariableRecords = objectVariableRecordsObject;

                auto objectVariableRecords = objectVariableRecordsObject->val<ObjectVariableRecord>();
                for (unsigned int i = 0; i < recordCount; i++) {
                    auto value = thread->consumeInstruction();
                    objectVariableRecords[i].variableIndex = static_cast<uint16_t>(value);
                    objectVariableRecords[i].condition = static_cast<uint16_t>(value >> 16);
                    objectVariableRecords[i].type = static_cast<ObjectVariableType>(thread->consumeInstruction());
                }

                if (thread->consumeInstruction()) {
                    c->thisContext = thread->thisContext();
                }

                thread->pushOpr(closure.unretainedPointer());
                thread->release(1);
                continue;
            }
            case INS_CLOSURE_BOX: {
                Object *closure = newObject(CL_CLOSURE);

                auto *c = closure->val<Closure>();

                c->function = functionTable[thread->consumeInstruction()];
                c->thisContext = thread->popOpr();
                thread->pushOpr(closure);
                continue;
            }
            case INS_CAPTURE_METHOD: {
                auto callee = thread->retain(thread->popOpr().object);

                Object *closureObject = newObject(CL_CLOSURE);
                auto *closure = closureObject->val<Closure>();

                EmojicodeInstruction vti = thread->consumeInstruction();
                closure->function = callee->klass->methodsVtable[vti];
                closure->thisContext = callee.unretainedPointer();
                thread->release(1);
                thread->pushOpr(closureObject);
                continue;
            }
            case INS_CAPTURE_TYPE_METHOD: {
                Object *closureObject = newObject(CL_CLOSURE);
                auto *closure = closureObject->val<Closure>();

                auto v = thread->popOpr();
                EmojicodeInstruction vti = thread->consumeInstruction();
                closure->function = v.klass->methodsVtable[vti];
                closure->thisContext = v;
                thread->pushOpr(closureObject);
                continue;
            }
            case INS_CAPTURE_CONTEXTED_FUNCTION: {
                Object *closureObject = newObject(CL_CLOSURE);
                auto *closure = closureObject->val<Closure>();

                EmojicodeInstruction vti = thread->consumeInstruction();
                closure->function = functionTable[vti];
                closure->thisContext = thread->popOpr();

                thread->pushOpr(closureObject);
                continue;
            }
        }
        error("Illegal bytecode instruction");
    }
}

}  // namespace Emojicode
