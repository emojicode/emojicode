//
//  Emojicode.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Emojicode_h
#define Emojicode_h

#include <cstdio>
#include "EmojicodeAPI.hpp"

extern Byte *currentHeap;
extern Byte *otherHeap;
void allocateHeap(void);

#ifndef heapSize
#define heapSize (512 * 1000 * 1000)  // 512 MB
#endif

struct Block {
    /// A pointer to the first instruction
    EmojicodeInstruction *instructions;
    /// The number of instructions in this block
    unsigned int instructionCount;
};

struct Function {
    /// Number of arguments taken by this function
    int argumentCount;
    /// Whether the method is native
    bool native;
    /// The frame size needed to execute this function.
    int frameSize;

    union {
        /** FunctionPointer pointer to execute the method. */
        FunctionFunctionPointer handler;
        Block block;
    };
};

/// The global class table
extern Class **classTable;
/// The global function table for function dispatch
extern Function **functionTable;

extern uint_fast16_t stringPoolCount;
extern Object **stringPool;

/** Whether the given pointer points into the heap. */
extern bool isPossibleObjectPointer(void *);

extern char **cliArguments;
extern int cliArgumentCount;

extern const char *packageDirectory;

struct Class {
    Class() {}
    Class(void (*mark)(Object *)) : instanceVariableCount(0), mark(mark) {}

    /** Returns true if @c cl or a superclass of @c cl conforms to the protocol. */
    bool conformsTo(int index) const;

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from) const;

    Function **methodsVtable;
    Function **initializersVtable;

    Function ***protocolsTable;
    uint_fast16_t protocolsOffset;
    uint_fast16_t protocolsMaxIndex;

    /** The class’s superclass */
    struct Class *superclass;

    /** The number of instance variables. */
    uint16_t instanceVariableCount;

    /** Marker FunctionPointer for GC */
    void (*mark)(Object *self);

    size_t size;
    size_t valueSize;
};

struct CapturedFunctionCall {
    Value callee;
    Function *function;
};

struct CaptureInformation {
    int size;
    int destination;
};

struct Closure {
    Block block;
    uint8_t argumentCount;
    uint8_t variableCount;
    int captureCount;
    Object *capturedVariables;
    Object *capturesInformation;
    Value thisContext;
};

/// Aborts the program with a run-time error
/// This function must only be used when recovery is impossible, i.e. unwrapping Nothingness, unknown instruction etc.
[[noreturn]] void error(const char *err, ...);

/// Reads a bytecode file
Function* readBytecode(FILE *in);

/** Determines whether the loading of a package was succesfull */
typedef enum {
    PACKAGE_LOADING_FAILED, PACKAGE_HEADER_NOT_FOUND, PACKAGE_INAPPROPRIATE_MAJOR, PACKAGE_INAPPROPRIATE_MINOR,
    PACKAGE_LOADED
} PackageLoadingState;

typedef Marker (*MarkerPointerForClass)(EmojicodeChar cl);
typedef uint_fast32_t (*SizeForClassFunction)(Class *cl, EmojicodeChar name);

char* packageError(void);

extern FunctionFunctionPointer sLinkingTable[100];
Marker markerPointerForClass(EmojicodeChar cl);
uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name);

#endif /* Emojicode_h */
