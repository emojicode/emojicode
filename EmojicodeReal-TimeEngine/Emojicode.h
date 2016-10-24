//
//  Emojicode.h
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef Emojicode_h
#define Emojicode_h

#define _GNU_SOURCE
#include "EmojicodeAPI.h"

//MARK: Stack

struct StackFrame {
    Something thisContext;
    uint8_t variableCount;
    void *returnPointer;
    void *returnFutureStack;
};

struct StackState {
    Byte *futureStack;
    Byte *stack;
};

/** Try to allocate a thread and a stack. */
Thread* allocateThread(void);

/** Removes the thread from the linked list. */
void removeThread(Thread *);

/** Marks all variables on the stack */
void stackMark(Thread *);

/**
 * The garbage collector.
 * Not thread-safe!
 */
void gc();

struct Thread {
    EmojicodeCoin *tokenStream;
    Something returnValue;
    bool returned;
    
    Byte *stackLimit;
    Byte *stackBottom;
    Byte *stack;
    Byte *futureStack;
    
    Thread *threadBefore;
    Thread *threadAfter;
};

extern Thread *lastThread;
extern int threads;

//MARK: VM

extern Byte *currentHeap;
extern Byte *otherHeap;
void allocateHeap(void);

#ifndef heapSize
#define heapSize (512 * 1000 * 1000) //512 MB
#endif

/** The class table */
extern Class **classTable;
extern Function **functionTable;

extern uint_fast16_t stringPoolCount;
extern Object **stringPool;

/** Whether the given pointer points into the heap. */
extern bool isPossibleObjectPointer(void *);

extern char **cliArguments;
extern int cliArgumentCount;

//MARK: Classes

struct Class {
    Function **methodsVtable;
    InitializerFunction **initializersVtable;
    
    Function ***protocolsTable;
    uint_fast16_t protocolsOffset;
    uint_fast16_t protocolsMaxIndex;
    
    /** The class’s superclass */
    struct Class *superclass;
    
    /** The number of instance variables. */
    uint16_t instanceVariableCount;
    uint16_t methodCount;
    uint16_t initializerCount;
    
    /** Deinitializer */
    void (*deconstruct)(void *);
    
    /** Marker FunctionPointer for GC */
    void (*mark)(Object *self);
    
    size_t size;
    size_t valueSize;
};

struct Function {
    /** Number of arguments. */
    uint8_t argumentCount;
    /** Whether the method is native */
    bool native;
    /** The number of variavles. */
    uint8_t variableCount;
    
    union {
        /** FunctionPointer pointer to execute the method. */
        FunctionFunctionPointer handler;
        struct {
            /** The method’s token stream */
            EmojicodeCoin *tokenStream;
            /** The number of tokens */
            uint32_t tokenCount;
        };
    };
};

struct InitializerFunction {
    /** Number of arguments. */
    uint8_t argumentCount;
    /** Whether the method is native */
    bool native;
    /** The number of variables. */
    uint8_t variableCount;
    
    union {
        /** FunctionPointer pointer to execute the method. */
        InitializerFunctionFunctionPointer handler;
        struct {
            /** The initializer’s token stream */
            EmojicodeCoin *tokenStream;
            /** The number of tokens */
            uint32_t tokenCount;
        };
    };
};

typedef struct {
    Something callee;
    Function *function;
} CapturedFunctionCall;

typedef struct {
    EmojicodeCoin *tokenStream;
    uint32_t coinCount;
    uint8_t argumentCount;
    uint8_t capturedVariablesCount;
    uint8_t variableCount;
    Object *capturedVariables;
    Something thisContext;
} Closure;

//MARK: Parsing

EmojicodeCoin consumeCoin(Thread *thread);

/** Parse a token */
Something parse(EmojicodeCoin coin, Thread *);

/** Throw a runtime error */
_Noreturn void error(char *err, ...);


//MARK: Object

Something objectGetVariable(Object *o, uint8_t index);

void objectSetVariable(Object *o, uint8_t index, Something value);

void objectDecrementVariable(Object *o, uint8_t index);
void objectIncrementVariable(Object *o, uint8_t index);


//MARK: Reading bytecode file

/** Reads all classes from the given bytecode file. Returns the class with the chequered flag. */
Function* readBytecode(FILE *in);


//MARK: Packages

/** Determines whether the loading of a package was succesfull */
typedef enum {
    PACKAGE_LOADING_FAILED, PACKAGE_HEADER_NOT_FOUND, PACKAGE_INAPPROPRIATE_MAJOR, PACKAGE_INAPPROPRIATE_MINOR,
    PACKAGE_LOADED
} PackageLoadingState;

typedef FunctionFunctionPointer (*FunctionFunctionPointerProvider)(EmojicodeChar cl, EmojicodeChar symbol, MethodType);
typedef InitializerFunctionFunctionPointer (*InitializerFunctionFunctionPointerProvider)(EmojicodeChar cl, EmojicodeChar symbol);
typedef Marker (*mpfc)(EmojicodeChar cl);
typedef Deinitializer (*dpfc)(EmojicodeChar cl);
typedef uint_fast32_t (*SizeForClassFunction)(Class *cl, EmojicodeChar name);

char* packageError(void);

FunctionFunctionPointer handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol, MethodType);
InitializerFunctionFunctionPointer handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol);
Marker markerPointerForClass(EmojicodeChar cl);
Deinitializer deinitializerPointerForClass(EmojicodeChar cl);
uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name);

#endif /* Emojicode_h */
