//
//  Emojicode.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Emojicode_h
#define Emojicode_h

#include "EmojicodeAPI.hpp"
#include <cstdio>

namespace Emojicode {

/// Used to signify the compiler that from the @c from -th instruction to the to @c to -th instruction the variable at
/// index @c variableIndex contains an object reference.
class ObjectVariableRecord {
public:
    unsigned int variableIndex;
    unsigned int condition;
    ObjectVariableType type;
};

class FunctionObjectVariableRecord : public ObjectVariableRecord {
public:
    int from;
    int to;
};

struct Block {
    /// A pointer to the first instruction
    EmojicodeInstruction *instructions;
    /// The number of instructions in this block
    unsigned int instructionCount;
};

struct Function {
    /// Number of arguments taken by this function
    int argumentCount;

    /// The frame size needed to execute this function.
    int frameSize;

    FunctionObjectVariableRecord *objectVariableRecords;
    unsigned int objectVariableRecordsCount;
    ContextType context;

    Block block;

    /// A native function connect to this function
    FunctionFunctionPointer handler;
};

struct ProtocolDispatchTable {
    Function ***protocolsTable;
    uint_fast16_t protocolsOffset;
    uint_fast16_t protocolsMaxIndex;

    Function* dispatch(uint_fast16_t protocolIndex, uint_fast16_t functionIndex) const {
        return protocolsTable[protocolIndex - protocolsOffset][functionIndex];
    }

    bool conformsTo(uint_fast16_t protocolIndex) const {
        if (protocolsTable == nullptr || protocolIndex < protocolsOffset || protocolIndex > protocolsMaxIndex) {
            return false;
        }
        return protocolsTable[protocolIndex - protocolsOffset] != nullptr;
    }
};

/// The global class table
extern Class **classTable;
/// The global function table for function dispatch
extern Function **functionTable;
/// The global protocol dispatch table table for value types
extern ProtocolDispatchTable *protocolDispatchTableTable;
extern uint32_t protocolDTTOffset;

extern uint_fast16_t stringPoolCount;
extern Object **stringPool;

extern char **cliArguments;
extern int cliArgumentCount;

extern const char *packageDirectory;

struct CaptureInformation {
    int size;
    int destination;
};

struct Closure {
    Function *function;
    unsigned int captureCount;
    unsigned int recordsCount;
    Object *capturedVariables;
    Object *capturesInformation;
    Object *objectVariableRecords;
    Value thisContext;
};

/// Aborts the program with a run-time error
/// This function must only be used when recovery is impossible, i.e. unwrapping Nothingness, unknown instruction etc.
[[noreturn]] void error(const char *err, ...);

typedef void (*PrepareClassFunction)(Class *cl, EmojicodeChar name);

extern FunctionFunctionPointer sLinkingTable[100];
void sPrepareClass(Class *klass, EmojicodeChar name);

}

#endif /* Emojicode_h */
