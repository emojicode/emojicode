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

inline void runFunctionPointerBlock(Thread *thread) {
    pauseForGC();

    while (thread->currentStackFrame()->executionPointer) {
        Box garbage;
        produce(thread, &garbage.type);
    }
}

inline Class* readClass(Thread *thread) {
    Value sth;
    produce(thread, &sth);
    return sth.klass;
}

inline double readDouble(Thread *thread) {
    EmojicodeInteger scale = ((EmojicodeInteger)thread->consumeInstruction() << 32) ^ thread->consumeInstruction();
    EmojicodeInteger exp = thread->consumeInstruction();

    return ldexp(static_cast<double>(scale)/PORTABLE_INTLEAST64_MAX, static_cast<int>(exp));
}

void Box::unwrapOptional() const {
    if (isNothingness()) {
        error("Unexpectedly found ✨ while unwrapping a 🍬.");
    }
}

void loadCapture(Closure *c, Thread *thread) {
    if (c->captureCount == 0) {
        return;
    }
    auto *cv = c->capturedVariables->val<Value>();
    auto *infop = c->capturesInformation->val<CaptureInformation>();
    for (unsigned int i = 0; i < c->captureCount; i++) {
        std::memcpy(thread->variableDestination(infop->destination), cv, infop->size * sizeof(Value));
        cv += infop->size;
        infop++;
    }
}

void executeCallableExtern(Object *callable, Value *args, size_t argsSize, Thread *thread, Value *destination) {
    auto *c = callable->val<Closure>();
    auto sf = thread->reserveFrame(c->thisContext, c->function->frameSize, c->function,
                                   destination, c->function->block.instructions);
    std::memcpy(sf->variableDestination(0), args, argsSize * sizeof(Value));
    thread->pushReservedFrame();
    loadCapture(c, thread);
    runFunctionPointerBlock(thread);
    thread->popStack();
}

void performFunction(Function *function, Value self, Thread *thread, Value *destination) {
    thread->pushStack(self, function->frameSize, function->argumentCount, function, destination,
                      function->block.instructions);
    runFunctionPointerBlock(thread);
    thread->popStack();
}

inline EmojicodeInteger normalizedBoxType(EmojicodeInteger type) {
    return (type & REMOTE_MASK) != 0 ? (type & ~REMOTE_MASK) : type;
}

void produce(Thread *thread, Value *destination) {
    switch (static_cast<Instructions>(thread->consumeInstruction())) {
        case INS_DISPATCH_METHOD: {
            Value sth;
            produce(thread, &sth);

            EmojicodeInstruction vti = thread->consumeInstruction();
            performFunction(sth.object->klass->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_TYPE_METHOD: {
            Value sth;
            produce(thread, &sth);

            EmojicodeInstruction vti = thread->consumeInstruction();
            performFunction(sth.klass->methodsVtable[vti], sth, thread, destination);
            return;
        }
        case INS_DISPATCH_PROTOCOL: {
            Value sth;
            produce(thread, &sth);

            EmojicodeInstruction pti = thread->consumeInstruction();
            EmojicodeInstruction vti = thread->consumeInstruction();

            auto type = sth.value[0].raw;
            if (type == T_OBJECT) {
                Object *o = sth.value[1].object;
                performFunction(o->klass->protocolTable.dispatch(pti, vti), sth.value[1], thread, destination);
            }
            else if ((type & REMOTE_MASK) != 0) {
                performFunction(protocolDispatchTableTable[(type & ~REMOTE_MASK) - protocolDTTOffset].dispatch(pti, vti),
                                sth.value[1].object->val<Value>(), thread, destination);
            }
            else {
                performFunction(protocolDispatchTableTable[type - protocolDTTOffset].dispatch(pti, vti),
                                sth.value + 1, thread, destination);
            }
            return;
        }
        case INS_NEW_OBJECT: {
            Class *klass = readClass(thread);
            Object *object = newObject(klass);
            Function *initializer = klass->initializersVtable[thread->consumeInstruction()];
            performFunction(initializer, object, thread, destination);
            return;
        }
        case INS_PRODUCE_TO_AND_GET_VT_REFERENCE: {
            Value *pd = thread->variableDestination(thread->consumeInstruction());
            produce(thread, pd);
            destination->value = pd;
            return;
        }
        case INS_INIT_VT: {
            EmojicodeInstruction c = thread->consumeInstruction();
            performFunction(functionTable[c], Value(destination), thread, nullptr);
            return;
        }
        case INS_DISPATCH_SUPER: {
            Class *klass = readClass(thread);
            EmojicodeInstruction vti = thread->consumeInstruction();

            performFunction(klass->methodsVtable[vti], thread->thisContext(), thread, destination);
            return;
        }
        case INS_CALL_CONTEXTED_FUNCTION: {
            Value sth;
            produce(thread, &sth);

            EmojicodeInstruction c = thread->consumeInstruction();
            performFunction(functionTable[c], sth, thread, destination);
            return;
        }
        case INS_CALL_FUNCTION: {
            EmojicodeInstruction c = thread->consumeInstruction();
            performFunction(functionTable[c], Value(), thread, destination);
            return;
        }
        case INS_SIMPLE_OPTIONAL_PRODUCE:
            produce(thread, destination + 1);
            destination->raw = T_OPTIONAL_VALUE;
            return;
        case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            if (box.type.raw != T_NOTHINGNESS) {
                std::memcpy(destination, &box.type, size * sizeof(Value));
            }
            else {
                destination->raw = T_NOTHINGNESS;
            }
            return;
        }
        case INS_SIMPLE_OPTIONAL_TO_BOX: {
            EmojicodeInstruction typeId = thread->consumeInstruction();
            produce(thread, destination);
            if (destination->raw != 0) {  // First value non-zero means a value
                destination->raw = typeId;
            }
            return;
        }
        case INS_BOX_PRODUCE: {
            auto id = thread->consumeInstruction();
            produce(thread, destination + 1);
            destination->raw = id;
            return;
        }
        case INS_UNBOX: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            std::memcpy(destination, &box.value1, size * sizeof(Value));
            return;
        }
        case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            if (box.type.raw != T_NOTHINGNESS) {
                destination->raw = T_OPTIONAL_VALUE;
                std::memcpy(destination + 1, box.value1.object->val<Value>(), size * sizeof(Value));
            }
            else {
                destination->raw = T_NOTHINGNESS;
            }
            return;
        }
        case INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE: {
            EmojicodeInstruction typeId = thread->consumeInstruction();
            auto size = thread->consumeInstruction();
            destination[1].object = newArray((size + 1) * sizeof(Value));
            auto value = destination[1].object->val<Value>();
            produce(thread, value);
            if (value->raw != T_NOTHINGNESS) {
                destination->raw = typeId;
                std::memmove(value, value + 1, size * sizeof(Value));
            }
            else {
                destination->raw = T_NOTHINGNESS;
            }
            return;
        }
        case INS_BOX_PRODUCE_REMOTE:
            destination->raw = thread->consumeInstruction();
            destination[1].object = newArray(thread->consumeInstruction() * sizeof(Value));
            produce(thread, destination[1].object->val<Value>());
            return;
        case INS_UNBOX_REMOTE: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            std::memcpy(destination, box.value1.object->val<Value>(), size * sizeof(Value));
            return;
        }
        case INS_GET_VT_REFERENCE_STACK:
            destination->value = thread->variableDestination(thread->consumeInstruction());
            return;
        case INS_GET_VT_REFERENCE_OBJECT: {
            destination->value = thread->thisObject()->variableDestination(thread->consumeInstruction());
            return;
        }
        case INS_GET_VT_REFERENCE_VT: {
            destination->value = thread->thisContext().value + thread->consumeInstruction();
            return;
        }
        case INS_GET_CLASS_FROM_INSTANCE: {
            Value a;
            produce(thread, &a);
            destination->klass = a.object->klass;
            return;
        }
        case INS_GET_CLASS_FROM_INDEX:
            destination->klass = classTable[thread->consumeInstruction()];
            return;
        case INS_GET_STRING_POOL:
            destination->object = stringPool[thread->consumeInstruction()];
            return;
        case INS_GET_TRUE:
            destination->raw = 1;
            return;
        case INS_GET_FALSE:
            destination->raw = 0;
            return;
        case INS_GET_32_INTEGER: {
            destination->raw = static_cast<EmojicodeInteger>(thread->consumeInstruction()) - INT32_MAX;
            return;
        }
        case INS_GET_64_INTEGER: {
            EmojicodeInteger a = thread->consumeInstruction();
            destination->raw = a << 32 | thread->consumeInstruction();
            return;
        }
        case INS_GET_DOUBLE:
            destination->doubl = readDouble(thread);
            return;
        case INS_GET_SYMBOL:
            destination->character = thread->consumeInstruction();
            return;
        case INS_GET_NOTHINGNESS:
            destination->raw = T_NOTHINGNESS;
            return;
        case INS_PRODUCE_WITH_STACK_DESTINATION: {
            EmojicodeInstruction index = thread->consumeInstruction();
            produce(thread, thread->variableDestination(index));
            return;
        }
        case INS_PRODUCE_WITH_OBJECT_DESTINATION: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *d = thread->thisObject()->variableDestination(index);
            produce(thread, d);
            return;
        }
        case INS_PRODUCE_WITH_VT_DESTINATION: {
            Value *d = thread->thisContext().value + thread->consumeInstruction();
            produce(thread, d);
            return;
        }
        case INS_INCREMENT:
            destination->raw++;
            return;
        case INS_DECREMENT:
            destination->raw--;
            return;
        case INS_COPY_SINGLE_STACK:
            *destination = thread->variable(thread->consumeInstruction());
            return;
        case INS_COPY_WITH_SIZE_STACK: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *source = thread->variableDestination(index);
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_SINGLE_OBJECT: {
            EmojicodeInstruction index = thread->consumeInstruction();
            *destination = *thread->thisObject()->variableDestination(index);
            return;
        }
        case INS_COPY_WITH_SIZE_OBJECT: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *source = thread->thisObject()->variableDestination(index);
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_SINGLE_VT:
            *destination = thread->thisContext().value[thread->consumeInstruction()];
            return;
        case INS_COPY_WITH_SIZE_VT: {
            Value *source = thread->thisContext().value + thread->consumeInstruction();
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_FROM_REFERENCE: {
            Value value;
            auto size = thread->consumeInstruction();
            produce(thread, &value);
            std::memcpy(destination, value.value, sizeof(Value) * size);
            return;
        }
            // Operators
        case INS_EQUAL_PRIMITIVE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw == b.raw;
            return;
        }
        case INS_EQUAL_SYMBOL: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.character == b.character;
            return;
        }
        case INS_SUBTRACT_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw - b.raw;
            return;
        }
        case INS_ADD_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw + b.raw;
            return;
        }
        case INS_MULTIPLY_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw * b.raw;
            return;
        }
        case INS_DIVIDE_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw / b.raw;
            return;
        }
        case INS_REMAINDER_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw % b.raw;
            return;
        }
        case INS_INVERT_BOOLEAN: {
            Value a;
            produce(thread, &a);
            destination->raw = !a.raw;
            return;
        }
        case INS_OR_BOOLEAN: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw || b.raw;
            return;
        }
        case INS_AND_BOOLEAN: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw && b.raw;
            return;
        }
        case INS_LESS_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw < b.raw;
            return;
        }
        case INS_GREATER_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw > b.raw;
            return;
        }
        case INS_GREATER_OR_EQUAL_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw >= b.raw;
            return;
        }
        case INS_LESS_OR_EQUAL_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw <= b.raw;
            return;
        }
        case INS_SAME_OBJECT: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.object == b.object;
            return;
        }
        case INS_IS_NOTHINGNESS: {
            Value a;
            produce(thread, &a);
            destination->raw = a.value->raw == T_NOTHINGNESS;
            return;
        }
        case INS_IS_ERROR: {
            Value a;
            produce(thread, &a);
            destination->raw = a.value->raw == T_ERROR;
            return;
        }
        case INS_EQUAL_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.doubl == b.doubl;
            return;
        }
        case INS_SUBTRACT_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->doubl = a.doubl - b.doubl;
            return;
        }
        case INS_ADD_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->doubl = a.doubl + b.doubl;
            return;
        }
        case INS_MULTIPLY_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->doubl = a.doubl * b.doubl;
            return;
        }
        case INS_DIVIDE_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->doubl = a.doubl / b.doubl;
            return;
        }
        case INS_LESS_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.doubl < b.doubl;
            return;
        }
        case INS_GREATER_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.doubl > b.doubl;
            return;
        }
        case INS_LESS_OR_EQUAL_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.doubl <= b.doubl;
            return;
        }
        case INS_GREATER_OR_EQUAL_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.doubl >= b.doubl;
            return;
        }
        case INS_REMAINDER_DOUBLE: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->doubl = fmod(a.doubl, b.doubl);
            return;
        }
        case INS_INT_TO_DOUBLE: {
            Value a;
            produce(thread, &a);
            destination->doubl = a.raw;
            return;
        }
        case INS_UNWRAP_SIMPLE_OPTIONAL: {
            Value value;
            produce(thread, &value);
            EmojicodeInteger n = thread->consumeInstruction();
            if (value.value->raw != T_NOTHINGNESS) {
                std::memcpy(destination, value.value + 1, n * sizeof(Value));
            }
            else {
                error("Unexpectedly found ✨ while unwrapping a 🍬.");
            }
            return;
        }
        case INS_UNWRAP_BOX_OPTIONAL: {
            Value sth;
            produce(thread, &sth);
            auto *box = reinterpret_cast<Box *>(sth.value);
            box->unwrapOptional();
            box->copyTo(destination);
            return;
        }
        case INS_ERROR_CHECK_SIMPLE_OPTIONAL: {
            Value value;
            produce(thread, &value);
            EmojicodeInteger n = thread->consumeInstruction();
            if (value.value->raw != T_ERROR) {
                std::memcpy(destination, value.value + 1, n * sizeof(Value));
            }
            else {
                error("Unexpectedly found 🚨 with value %d.", value.value[1].raw);
            }
            return;
        }
        case INS_ERROR_CHECK_BOX_OPTIONAL: {
            Value sth;
            produce(thread, &sth);
            auto *box = reinterpret_cast<Box *>(sth.value);
            if (box->type.raw != T_ERROR) {
                box->copyTo(destination);
            }
            else {
                error("Unexpectedly found 🚨 with value %d.", box->value1.raw);
            }
            return;
        }
        case INS_CONDITIONAL_PRODUCE_BOX: {
            Value sth;
            produce(thread, &sth);
            auto *box = reinterpret_cast<Box *>(sth.value);
            EmojicodeInstruction index = thread->consumeInstruction();
            if ((destination->raw = (box->type.raw != T_NOTHINGNESS))) {
                box->copyTo(thread->variableDestination(index));
            }
            return;
        }
        case INS_CONDITIONAL_PRODUCE_SIMPLE_OPTIONAL: {
            Value value;
            produce(thread, &value);
            EmojicodeInstruction index = thread->consumeInstruction();
            EmojicodeInstruction n = thread->consumeInstruction();
            if ((destination->raw = (value.value->raw != T_NOTHINGNESS))) {
                std::memcpy(thread->variableDestination(index), value.value + 1, n * sizeof(Value));
            }
            return;
        }
        case INS_THIS:
            *destination = thread->thisContext();
            return;
        case INS_SUPER_INITIALIZER: {
            Class *klass = readClass(thread);
            Object *o = thread->thisObject();

            EmojicodeInstruction vti = thread->consumeInstruction();
            Function *initializer = klass->initializersVtable[vti];

            performFunction(initializer, o, thread, destination);
            return;
        }
        case INS_DOWNCAST_TO_CLASS: {
            produce(thread, destination + 1);
            Class *klass = readClass(thread);
            destination->raw = destination[1].object->klass->inheritsFrom(klass);
            return;
        }
        case INS_CAST_TO_CLASS: {
            Box box;
            produce(thread, &box.type);
            Class *klass = readClass(thread);
            if (box.type.raw == T_OBJECT && !box.isNothingness() && box.value1.object->klass->inheritsFrom(klass)) {
                destination[0].raw = T_OPTIONAL_VALUE;
                destination[1] = box.value1;
                return;
            }
            destination->makeNothingness();
            return;
        }
        case INS_CAST_TO_PROTOCOL: {
            produce(thread, destination);
            EmojicodeInstruction pi = thread->consumeInstruction();
            auto box = reinterpret_cast<Box *>(destination);
            if (!(!box->isNothingness() &&
                  ((box->type.raw == T_OBJECT && box->value1.object->klass->protocolTable.conformsTo(pi)) ||
                    protocolDispatchTableTable[normalizedBoxType(box->type.raw) - protocolDTTOffset].conformsTo(pi)))) {
                destination->makeNothingness();
            }
            return;
        }
        case INS_CAST_TO_VALUE_TYPE: {
            produce(thread, destination);
            EmojicodeInstruction id = thread->consumeInstruction();
            if (destination->raw != id) {
                destination->makeNothingness();
            }
            return;
        }
        case INS_OPT_DICTIONARY_LITERAL: {
            auto dico = thread->retain(newObject(CL_DICTIONARY));
            dictionaryInit(dico->val<EmojicodeDictionary>());

            EmojicodeInstruction variableSlot = thread->consumeInstruction();
            EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + thread->consumeInstruction();
            while (thread->currentStackFrame()->executionPointer < end) {
                Value key;
                produce(thread, &key);
                auto keyObject = thread->retain(key.object);
                produce(thread, thread->variableDestination(variableSlot));
                dictionaryPutVal(dico, keyObject, thread)->copy(thread->variableDestination(variableSlot));
                thread->release(1);
            }
            destination->object = dico.unretainedPointer();
            thread->release(1);
            return;
        }
        case INS_OPT_LIST_LITERAL: {
            auto list = thread->retain(newObject(CL_LIST));

            EmojicodeInstruction variableSlot = thread->consumeInstruction();
            EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + thread->consumeInstruction();
            while (thread->currentStackFrame()->executionPointer < end) {
                produce(thread, thread->variableDestination(variableSlot));
                listAppendDestination(list, thread)->copy(thread->variableDestination(variableSlot));
            }

            destination->object = list.unretainedPointer();
            thread->release(1);
            return;
        }
        case INS_OPT_STRING_CONCATENATE_LITERAL: {
            EmojicodeInstruction stringCount = thread->consumeInstruction();

            size_t bufferSize = 10;
            size_t length = 0;
            auto characters = thread->retain(newArray(bufferSize * sizeof(EmojicodeChar)));

            for (EmojicodeInstruction i = 0; i < stringCount; i++) {
                Value v;
                produce(thread, &v);
                auto stringObject = thread->retain(v.object);
                auto size = static_cast<size_t>(stringObject->val<String>()->length);
                if (bufferSize - length < size) {
                    bufferSize += size - (bufferSize - length);
                    characters = resizeArray(characters.unretainedPointer(), bufferSize * sizeof(EmojicodeChar), thread);
                }
                EmojicodeChar *dest = characters->val<EmojicodeChar>() + length;
                auto string = stringObject->val<String>();
                std::memcpy(dest, string->characters(), string->length * sizeof(EmojicodeChar));
                length += stringObject->val<String>()->length;
                thread->release(1);
            }

            Object *object = newObject(CL_STRING);
            auto *string = object->val<String>();
            string->length = length;
            string->charactersObject = characters.unretainedPointer();

            destination->object = object;
            thread->release(1);
            return;
        }
        case INS_BINARY_AND_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw & b.raw;
            return;
        }
        case INS_BINARY_OR_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw | b.raw;
            return;
        }
        case INS_BINARY_XOR_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw ^ b.raw;
            return;
        }
        case INS_BINARY_NOT_INTEGER: {
            Value a;
            produce(thread, &a);
            destination->raw = ~a.raw;
            return;
        }
        case INS_SHIFT_LEFT_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw << b.raw;
            return;
        }
        case INS_SHIFT_RIGHT_INTEGER: {
            Value a;
            Value b;
            produce(thread, &a);
            produce(thread, &b);
            destination->raw = a.raw >> b.raw;
            return;
        }
        case INS_RETURN:
            produce(thread, thread->currentStackFrame()->destination);
            thread->returnFromFunction();
            return;
        case INS_ERROR:
            thread->currentStackFrame()->destination->raw = T_ERROR;
            produce(thread, thread->currentStackFrame()->destination + 1);
            thread->returnFromFunction();
            return;
        case INS_RETURN_WITHOUT_VALUE:
            thread->returnFromFunction();
            return;
        case INS_JUMP_FORWARD:
            thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
            return;
        case INS_JUMP_FORWARD_IF: {
            Value sth;
            produce(thread, &sth);
            if (sth.raw) {
                thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
            }
            else {
                thread->consumeInstruction();
            }
            return;
        }
        case INS_JUMP_BACKWARD_IF: {
            Value sth;
            produce(thread, &sth);
            if (sth.raw) {
                auto a = thread->consumeInstruction();
                thread->currentStackFrame()->executionPointer -= a;
            }
            else {
                thread->consumeInstruction();
            }
            return;
        }
        case INS_JUMP_FORWARD_IF_NOT: {
            Value sth;
            produce(thread, &sth);
            if (!sth.raw) {
                thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
            }
            else {
                thread->consumeInstruction();
            }
            return;
        }
        case INS_JUMP_BACKWARD_IF_NOT: {
            Value sth;
            produce(thread, &sth);
            if (!sth.raw) {
                thread->currentStackFrame()->executionPointer -= thread->consumeInstruction();
            }
            else {
                thread->consumeInstruction();
            }
            return;
        }
        case INS_TRANSFER_CONTROL_TO_NATIVE:
            thread->currentStackFrame()->function->handler(thread);
            return;
        case INS_EXECUTE_CALLABLE: {
            Value sth;
            produce(thread, &sth);

            auto *c = sth.object->val<Closure>();
            thread->pushStack(c->thisContext, c->function->frameSize, c->function->argumentCount, c->function,
                              destination, c->function->block.instructions);
            loadCapture(c, thread);
            runFunctionPointerBlock(thread);
            thread->popStack();
            return;
        }
        case INS_CLOSURE: {
            auto closure = thread->retain(newObject(CL_CLOSURE));

            auto *c = closure->val<Closure>();

            c->function = functionTable[thread->consumeInstruction()];
            c->captureCount = thread->consumeInstruction();
            EmojicodeInteger size = thread->consumeInstruction();

            Object *captures = newArray(sizeof(Value) * size);
            c = closure->val<Closure>();
            c->capturedVariables = captures;
            Object *infoo = newArray(sizeof(CaptureInformation) * c->captureCount);
            c = closure->val<Closure>();
            c->capturesInformation = infoo;

            auto *t = c->capturedVariables->val<Value>();
            auto *info = c->capturesInformation->val<CaptureInformation>();
            for (unsigned int i = 0; i < c->captureCount; i++) {
                EmojicodeInstruction index = thread->consumeInstruction();
                EmojicodeInstruction size = thread->consumeInstruction();
                info->destination = thread->consumeInstruction();
                (info++)->size = size;
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

            destination->object = closure.unretainedPointer();
            thread->release(1);
            return;
        }
        case INS_CLOSURE_BOX: {
            auto destFn = thread->consumeInstruction();
            Value value;
            produce(thread, &value);

            auto closure = newObject(CL_CLOSURE);

            auto *c = closure->val<Closure>();
            c->function = functionTable[destFn];
            c->thisContext = value;
            destination->object = closure;
            return;
        }
        case INS_CAPTURE_METHOD: {
            Value sth;
            produce(thread, &sth);
            auto callee = thread->retain(sth.object);

            Object *closureObject = newObject(CL_CLOSURE);
            auto *closure = closureObject->val<Closure>();

            EmojicodeInstruction vti = thread->consumeInstruction();
            closure->function = callee->klass->methodsVtable[vti];
            closure->thisContext = callee.unretainedPointer();
            thread->release(1);
            destination->object = closureObject;
            return;
        }
        case INS_CAPTURE_TYPE_METHOD: {
            Value sth;
            produce(thread, &sth);

            Object *closureObject = newObject(CL_CLOSURE);
            auto *closure = closureObject->val<Closure>();

            EmojicodeInstruction vti = thread->consumeInstruction();
            closure->function = sth.klass->methodsVtable[vti];
            closure->thisContext = sth;
            destination->object = closureObject;
            return;
        }
        case INS_CAPTURE_CONTEXTED_FUNCTION: {
            Value sth;
            produce(thread, &sth);

            Object *closureObject = newObject(CL_CLOSURE);
            auto *closure = closureObject->val<Closure>();

            EmojicodeInstruction vti = thread->consumeInstruction();
            closure->function = functionTable[vti];
            closure->thisContext = sth;

            destination->object = closureObject;
            return;
        }
    }
    error("Illegal bytecode instruction");
}

}
