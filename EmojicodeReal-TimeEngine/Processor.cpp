//
//  Processor.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Processor.hpp"
#include <cmath>
#include <cstring>
#include <functional>
#include <thread>
#include "../EmojicodeInstructions.h"
#include "Thread.hpp"
#include "Class.hpp"
#include "EmojicodeList.h"
#include "EmojicodeString.h"
#include "EmojicodeDictionary.h"

static void passBlock(Thread *thread) {
    thread->currentStackFrame()->executionPointer += thread->consumeInstruction();
}

static bool runBlock(Thread *thread) {
    EmojicodeInstruction length = thread->consumeInstruction();  // Length of the block

    EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + length;
    while (thread->currentStackFrame()->executionPointer < end) {
        Box garbage;
        produce(thread, &garbage.type);

        pauseForGC();

        if (thread->currentStackFrame()->executionPointer == nullptr) {
            return true;
        }
    }

    return false;
}

static void runFunctionPointerBlock(Thread *thread, uint32_t length) {
    EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + length;
    while (thread->currentStackFrame()->executionPointer < end) {
        Box garbage;
        produce(thread, &garbage.type);

        pauseForGC();

        if (thread->currentStackFrame()->executionPointer == nullptr) {
            return;
        }
    }
    return;
}

Class* readClass(Thread *thread) {
    Value sth;
    produce(thread, &sth);
    return sth.klass;
}

static double readDouble(Thread *thread) {
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
    Value *cv = static_cast<Value *>(c->capturedVariables->value);
    CaptureInformation *infop = static_cast<CaptureInformation *>(c->capturesInformation->value);
    for (int i = 0; i < c->captureCount; i++) {
        std::memcpy(thread->variableDestination(infop->destination), cv, infop->size * sizeof(Value));
        cv += infop->size;
        infop++;
    }
}

void executeCallableExtern(Object *callable, Value *args, Thread *thread, Value *destination) {
    if (callable->klass == CL_CAPTURED_FUNCTION_CALL) {
        CapturedFunctionCall *cmc = static_cast<CapturedFunctionCall *>(callable->value);
        Function *method = cmc->function;

        if (method->native) {
            auto sf = thread->reserveFrame(cmc->callee, method->frameSize, method, nullptr, nullptr);
            std::memcpy(sf->variableDestination(0), args, method->argumentCount * sizeof(Value));
            thread->pushReservedFrame();
            method->handler(thread, destination);
        }
        else {
            auto sf = thread->reserveFrame(cmc->callee, method->frameSize, method, destination,
                                            method->block.instructions);
            memcpy(sf->variableDestination(0), args, method->argumentCount * sizeof(Value));
            thread->pushReservedFrame();

            runFunctionPointerBlock(thread, method->block.instructionCount);
        }
    }
    else {
        Closure *c = static_cast<Closure *>(callable->value);

        // TODO: Won’t work with real vts
        // TODO: WRONG nullptr
        auto sf = thread->reserveFrame(c->thisContext, c->variableCount, nullptr, destination, c->block.instructions);
        memcpy(sf->variableDestination(0), args, c->argumentCount * sizeof(Value));
        thread->pushReservedFrame();

        loadCapture(c, thread);

        runFunctionPointerBlock(thread, c->block.instructionCount);
    }
    thread->popStack();
}

bool performInitializer(Class *klass, Function *initializer, Object *object, Thread *thread) {
    Value returned = EmojicodeInteger(1);
    if (initializer->native) {
        thread->pushStack(object, initializer->frameSize, initializer->argumentCount, initializer,
                          &returned, nullptr);
        initializer->handler(thread, &returned);

        if (!returned.raw) {
            thread->popStack();
            return false;
        }
    }
    else {
        thread->pushStack(Value(object), initializer->frameSize, initializer->argumentCount, initializer,
                          &returned, initializer->block.instructions);

        EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + initializer->block.instructionCount;
        while (thread->currentStackFrame()->executionPointer < end) {
            Value garbage;
            produce(thread, &garbage);

            if (!returned.raw) {
                thread->popStack();
                return false;
            }
        }
    }
    object = thread->getThisObject();
    thread->popStack();
    return true;
}

void performFunction(Function *function, Value self, Thread *thread, Value *destination) {
    if (function->native) {
        thread->pushStack(self, function->frameSize, function->argumentCount, function, destination, nullptr);
        function->handler(thread, destination);
    }
    else {
        thread->pushStack(self, function->frameSize, function->argumentCount, function, destination,
                          function->block.instructions);
        runFunctionPointerBlock(thread, function->block.instructionCount);
    }
    thread->popStack();
}

void produce(Thread *thread, Value *destination) {
    switch ((EmojicodeInstructionConstants)thread->consumeInstruction()) {
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
            performInitializer(klass, initializer, object, thread);
            destination->object = object;
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

            performFunction(klass->methodsVtable[vti], thread->getThisContext(), thread, destination);
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
            destination->raw = 1;
            produce(thread, destination + 1);
            return;
        case INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            if (box.type.raw != T_NOTHINGNESS) std::memcpy(destination, &box.type, size * sizeof(Value));
            else destination->raw = 0;
            return;
        }
        case INS_SIMPLE_OPTIONAL_TO_BOX: {
            EmojicodeInstruction typeId = thread->consumeInstruction();
            produce(thread, destination);
            if (destination->raw) destination->raw = typeId;  // First value non-zero means a value
            return;
        }
        case INS_BOX_PRODUCE:
            destination->raw = thread->consumeInstruction();
            produce(thread, destination + 1);
            return;
        case INS_UNBOX: {
            Box box;
            EmojicodeInstruction size = thread->consumeInstruction();
            produce(thread, &box.type);
            std::memcpy(destination, &box.value1, size * sizeof(Value));
            return;
        }
        case INS_GET_VT_REFERENCE_STACK:
            destination->value = thread->variableDestination(thread->consumeInstruction());
            return;
        case INS_GET_VT_REFERENCE_OBJECT: {
            destination->value = thread->getThisObject()->variableDestination(thread->consumeInstruction());
            return;
        }
        case INS_GET_VT_REFERENCE_VT: {
            destination->value = thread->getThisContext().value + thread->consumeInstruction();
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
            destination->raw = 0;
            return;
        case INS_PRODUCE_WITH_STACK_DESTINATION: {
            EmojicodeInstruction index = thread->consumeInstruction();
            produce(thread, thread->variableDestination(index));
            return;
        }
        case INS_PRODUCE_WITH_OBJECT_DESTINATION: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *d = thread->getThisObject()->variableDestination(index);
            produce(thread, d);
            return;
        }
        case INS_PRODUCE_WITH_VT_DESTINATION: {
            Value *d = thread->getThisContext().value + thread->consumeInstruction();
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
            *destination = thread->getVariable(thread->consumeInstruction());
            return;
        case INS_COPY_WITH_SIZE_STACK: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *source = thread->variableDestination(index);
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_SINGLE_OBJECT: {
            EmojicodeInstruction index = thread->consumeInstruction();
            *destination = *thread->getThisObject()->variableDestination(index);
            return;
        }
        case INS_COPY_WITH_SIZE_OBJECT: {
            EmojicodeInstruction index = thread->consumeInstruction();
            Value *source = thread->getThisObject()->variableDestination(index);
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_SINGLE_VT:
            *destination = thread->getThisContext().value[thread->consumeInstruction()];
            return;
        case INS_COPY_WITH_SIZE_VT: {
            Value *source = thread->getThisContext().value + thread->consumeInstruction();
            std::memcpy(destination, source, sizeof(Value) * thread->consumeInstruction());
            return;
        }
        case INS_COPY_REFERENCE: {
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
            Box *box = reinterpret_cast<Box *>(sth.value);
            box->unwrapOptional();
            box->copyTo(destination);
            return;
        }
        case INS_CONDITIONAL_PRODUCE_BOX: {
            Value sth;
            produce(thread, &sth);
            Box *box = reinterpret_cast<Box *>(sth.value);
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
        case INS_GET_THIS:
            *destination = thread->getThisContext();
            return;
        case INS_SUPER_INITIALIZER: {
            Class *klass = readClass(thread);
            Object *o = thread->getThisObject();

            EmojicodeInstruction vti = thread->consumeInstruction();
            Function *initializer = klass->initializersVtable[vti];

            performInitializer(klass, initializer, o, thread);
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
                destination[0].raw = 1;
                destination[1] = box.value1;
                return;
            }
            destination[0].raw = 0;
            return;
        }
        case INS_CAST_TO_PROTOCOL: {
            produce(thread, destination);
            EmojicodeInstruction pi = thread->consumeInstruction();
            auto box = reinterpret_cast<Box *>(destination);
            if (!(!box->isNothingness() &&
                  ((box->type.raw == T_OBJECT && box->value1.object->klass->protocolTable.conformsTo(pi)) ||
                    protocolDispatchTableTable[box->type.raw].conformsTo(pi)))) {
                destination[0].raw = 0;
            }
            return;
        }
        case INS_CAST_TO_VALUE_TYPE: {
            Box box;
            produce(thread, &box.type);
            EmojicodeInstruction id = thread->consumeInstruction();
            if (box.type.raw != id) {
                destination->makeNothingness();
            }
            else {
                destination->raw = 1;
                box.copyContentTo(destination + 1);
            }
            return;
        }
        case 0x50: {
            Object *const &dico = thread->retain(newObject(CL_DICTIONARY));
            dictionaryInit(thread);

            EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + thread->consumeInstruction();
            while (thread->currentStackFrame()->executionPointer < end) {
                Value key;
                produce(thread, &key);
                Box sth;
                produce(thread, &sth.type);

                dictionaryPutVal(dico, key.object, sth, thread);
            }
            destination->object = dico;
            thread->release(1);
            return;
        }
        case 0x51: {
            Object *const &list = thread->retain(newObject(CL_LIST));

            EmojicodeInstruction *end = thread->currentStackFrame()->executionPointer + thread->consumeInstruction();
            while (thread->currentStackFrame()->executionPointer < end) {
                produce(thread,
                        reinterpret_cast<Value *>(listAppendDestination(list, thread)));
            }

            destination->object = list;
            thread->release(1);
            return;
        }
        case 0x52: {
            EmojicodeInstruction stringCount = thread->consumeInstruction();

            size_t bufferSize = 10;
            size_t length = 0;
            auto characters = std::ref(thread->retain(newArray(bufferSize * sizeof(EmojicodeChar))));

            for (int i = 0; i < stringCount; i++) {
                Value v;
                produce(thread, &v);
                String *string = static_cast<String *>(v.object->value);
                if (bufferSize - length < string->length) {
                    bufferSize += static_cast<size_t>(string->length) - (bufferSize - length);
                    thread->release(1);
                    characters = std::ref(thread->retain(resizeArray(characters, bufferSize * sizeof(EmojicodeChar))));
                }
                EmojicodeChar *dest = static_cast<EmojicodeChar *>(characters.get()->value) + length;
                std::memcpy(dest, string->characters->value, string->length * sizeof(EmojicodeChar));
                length += string->length;
            }

            Object *object = newObject(CL_STRING);
            String *string = static_cast<String *>(object->value);
            string->length = length;
            string->characters = characters;

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
            thread->currentStackFrame()->executionPointer = nullptr;
            return;
        case INS_REPEAT_WHILE: {
            EmojicodeInstruction *beginPosition = thread->currentStackFrame()->executionPointer;
            Value sth;
            while (produce(thread, &sth), sth.raw) {
                if (runBlock(thread)) {
                    return;
                }
                thread->currentStackFrame()->executionPointer = beginPosition;
            }
            passBlock(thread);
            return;
        }
        case INS_IF: {
            EmojicodeInstruction length = thread->consumeInstruction();
            EmojicodeInstruction *ifEnd = thread->currentStackFrame()->executionPointer + length;

            Value sth;
            produce(thread, &sth);
            if (sth.raw) {  // Main if
                if (runBlock(thread)) {
                    return;
                }
                thread->currentStackFrame()->executionPointer = ifEnd;
            }
            else {
                passBlock(thread);

                EmojicodeInstruction ins;
                while ((ins = thread->consumeInstruction())) {
                    if (ins == 1) {  // Else If
                        produce(thread, &sth);
                        if (sth.raw) {
                            if (runBlock(thread)) {
                                return;
                            }
                            thread->currentStackFrame()->executionPointer = ifEnd;
                            return;
                        }
                        passBlock(thread);
                    }
                    else {  // Else
                        runBlock(thread);
                        return;
                    }
                }
            }
            return;
        }
        case 0x65: {
            EmojicodeInstruction variable = thread->consumeInstruction();

            Value losm;
            produce(thread, &losm);

            EmojicodeInstruction listObjectVariable = thread->consumeInstruction();
            *thread->variableDestination(listObjectVariable) = losm;
            List *list = static_cast<List *>(losm.object->value);

            EmojicodeInstruction *begin = thread->currentStackFrame()->executionPointer;

            for (size_t i = 0; i < (list = static_cast<List *>(thread->getVariable(listObjectVariable).object->value))->count; i++) {
                list->elements()[i].copyTo(thread->variableDestination(variable));
                reinterpret_cast<Box *>(thread->variableDestination(variable))->unwrapOptional();

                if (runBlock(thread)) {
                    return;
                }
                thread->currentStackFrame()->executionPointer = begin;
            }
            passBlock(thread);
            return;
        }
        case 0x66: {
            EmojicodeInstruction variable = thread->consumeInstruction();
            Value sth;
            produce(thread, &sth);
            EmojicodeInstruction *begin = thread->currentStackFrame()->executionPointer;
            for (EmojicodeInteger i = sth.value[0].raw; i != sth.value[1].raw; i += sth.value[2].raw) {
                thread->variableDestination(variable)->raw = i;

                if (runBlock(thread)) {
                    return;
                }
                thread->currentStackFrame()->executionPointer = begin;
            }
            passBlock(thread);
            return;
        }
        case INS_EXECUTE_CALLABLE: {
            Value sth;
            produce(thread, &sth);
            Object *callable = sth.object;
            if (callable->klass == CL_CAPTURED_FUNCTION_CALL) {
                CapturedFunctionCall *cmc = static_cast<CapturedFunctionCall *>(callable->value);
                performFunction(cmc->function, cmc->callee, thread, destination);
                return;
            }
            else {
                Closure *c = static_cast<Closure *>(callable->value);
                // TODO: wrong nullptr
                thread->pushStack(c->thisContext, c->variableCount, c->argumentCount, nullptr, destination,
                                  c->block.instructions);

                loadCapture(c, thread);

                runFunctionPointerBlock(thread, c->block.instructionCount);
                thread->popStack();
                return;
            }
        }
        case INS_CLOSURE: {
            Object *const &closure = thread->retain(newObject(CL_CLOSURE));

            Closure *c = static_cast<Closure *>(closure->value);

            c->variableCount = thread->consumeInstruction();
            c->block.instructionCount = thread->consumeInstruction();
            c->block.instructions = thread->currentStackFrame()->executionPointer;
            thread->currentStackFrame()->executionPointer += c->block.instructionCount;

            EmojicodeInstruction argumentCount = thread->consumeInstruction();
            c->argumentCount = argumentCount;

            c->captureCount = thread->consumeInstruction();
            EmojicodeInteger size = thread->consumeInstruction();

            Object *captures = newArray(sizeof(Value) * size);
            c = static_cast<Closure *>(closure->value);
            c->capturedVariables = captures;
            Object *infoo = newArray(sizeof(CaptureInformation) * c->captureCount);
            c = static_cast<Closure *>(closure->value);
            c->capturesInformation = infoo;

            Value *t = static_cast<Value *>(c->capturedVariables->value);
            CaptureInformation *info = static_cast<CaptureInformation *>(c->capturesInformation->value);
            for (int i = 0; i < c->captureCount; i++) {
                EmojicodeInteger index = thread->consumeInstruction();
                EmojicodeInteger size = thread->consumeInstruction();
                info->destination = thread->consumeInstruction();
                (info++)->size = size;
                std::memcpy(t, thread->variableDestination(index), size * sizeof(Value));
                t += size;
            }

            if (argumentCount >> 16)
                c->thisContext = thread->getThisContext();

            destination->object = closure;
            thread->release(1);
            return;
        }
        case INS_CAPTURE_METHOD: {
            Value sth;
            produce(thread, &sth);
            Object *const &callee = thread->retain(sth.object);

            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = static_cast<CapturedFunctionCall *>(cmco->value);

            EmojicodeInstruction vti = thread->consumeInstruction();
            cmc->function = callee->klass->methodsVtable[vti];
            cmc->callee.object = callee;
            thread->release(1);
            destination->object = cmco;
            return;
        }
        case INS_CAPTURE_TYPE_METHOD: {
            Value sth;
            produce(thread, &sth);

            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = static_cast<CapturedFunctionCall *>(cmco->value);

            EmojicodeInstruction vti = thread->consumeInstruction();
            cmc->function = sth.klass->methodsVtable[vti];
            cmc->callee.klass = sth.klass;
            destination->object = cmco;
            return;
        }
        case INS_CAPTURE_CONTEXTED_FUNCTION: {
            Value sth;
            produce(thread, &sth);

            Object *cmco = newObject(CL_CAPTURED_FUNCTION_CALL);
            CapturedFunctionCall *cmc = static_cast<CapturedFunctionCall *>(cmco->value);

            EmojicodeInstruction vti = thread->consumeInstruction();
            cmc->function = functionTable[vti];
            cmc->callee = sth;

            destination->object = cmco;
            return;
        }
    }
    error("Illegal bytecode instruction");
}
