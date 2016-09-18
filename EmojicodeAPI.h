//
//  EmojicodeAPI.h
//  Emojicode
//
//  Created by Theo Weidmann on 12.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

/*
 * Welcome to Emojicode!
 * This API file is the point to get started if you want to write a package (or something else interfacing or using Emojicode).
 * You might need to also include `EmojicodeString.h`, `EmojicodeList.h` and `EmojicodeDictionary.h`. 
 * Follow the Emojicode style guide.
 */

#ifndef EmojicodeAPI_h
#define EmojicodeAPI_h

#include "EmojicodeShared.h"
#include <pthread.h>

typedef struct Class Class;
typedef struct Function Function;
typedef struct InitializerFunction InitializerFunction;
typedef struct List List;
typedef struct Thread Thread;
typedef struct StackFrame StackFrame;
typedef struct StackState StackState;

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_ERROR;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_CAPTURED_FUNCTION_CALL;
extern Class *CL_CLOSURE;
extern Class *CL_RANGE;
extern Class *CL_ARRAY;

typedef struct Object {
    /** The object’s class. */
    Class *class;
    /** The size of this object: the size of the Object struct and the value area. */
    size_t size;
    /** The objects garabage collection state. Do not touch this field! */
    struct Object *newLocation;
    /**
     * A pointer to the allocated @c value area. This area is as large as specified in the class.
     * @warning Never change the content of this variable as the Garbage Collector updates this field
     * in each cycle.
     */
    void *value;
} Object;

#define T_OBJECT 0
#define T_INTEGER 1
#define T_BOOLEAN 2
#define T_SYMBOL 3
#define T_DOUBLE 4
#define T_CLASS 5

typedef uint_fast8_t Type;
typedef unsigned char Byte;

/** Either an object reference or a primitive value. */
typedef struct {
    /** The type of the primitive or whether it contains an object reference. */
    Type type;
    union {
        EmojicodeInteger raw;
        double doubl;
        Object *object;
        Class *eclass;
    };
} Something;

#if __SIZEOF_DOUBLE__ != 8
#warning Double does not match the size of an 64-bit integer
#endif

#define somethingObject(o) ((Something){T_OBJECT, .object = (o)})
#define somethingInteger(o) ((Something){T_INTEGER, (o)})
#define somethingSymbol(o) ((Something){T_SYMBOL, (o)})
#define somethingBoolean(o) ((Something){T_BOOLEAN, (o)})
#define somethingDouble(o) ((Something){T_DOUBLE, .doubl = (o)})
#define somethingClass(o) ((Something){T_CLASS, .eclass = (o)})
#define EMOJICODE_TRUE ((Something){T_BOOLEAN, 1})
#define EMOJICODE_FALSE ((Something){T_BOOLEAN, 0})
#define NOTHINGNESS ((Something){T_OBJECT, .object = NULL})

#define unwrapInteger(o) ((o).raw)
#define unwrapBool(o) ((o).raw > 0)
#define unwrapSymbol(o) ((EmojicodeChar)(o).raw)
#define unwrapDouble(o) ((o).doubl)

/** Whether this thing is Nothingness. */
extern bool isNothingness(Something sth);

/** Whether this thing is a reference to a valid object. */
extern bool isRealObject(Something sth);

//MARK: Built In Classes

typedef struct String {
    /** The number of code points in @c characters. Strings are not null terminated! */
    EmojicodeInteger length;
    /** The characters of this string. Strings are not null terminated! */
    Object *characters;
} String;

typedef struct {
    EmojicodeInteger length;
    char *bytes;
    Object *bytesObject;
} Data;

typedef struct {
    const char *message;
    EmojicodeInteger code;
} EmojicodeError;

typedef struct {
    EmojicodeInteger start;
    EmojicodeInteger stop;
    EmojicodeInteger step;
} EmojicodeRange;

extern Object* newError(const char *message, int code);
extern void rangeSetDefaultStep(EmojicodeRange *range);


//MARK: Callables

/** You can use this function to call a callable object. It’s internally Garbage-Collector safe. */
extern Something executeCallableExtern(Object *callable, Something *args, Thread *thread);


//MARK: Primitive Objects

/**
 * Allocates a new object for the given class.
 * The @c value field will point to a value area as large as specified for the given class.
 * @param class The class of the object.
 * @warning GC-invoking
 */
extern Object* newObject(Class *class);

/**
 * Multiplies @c items by @c itemSize and terminates the program with an error if an integer
 * overflow occured.
 */
extern size_t sizeCalculationWithOverflowProtection(size_t items, size_t itemSize);

/** 
 * Allocates an array with a value area of the given size.
 * @param size The size of the value area.
 * @warning GC-invoking
 */
extern Object* newArray(size_t size);

/**
 * Tries to resize the given array object to the given size and returns a pointer to the resized array.
 * @param array An array object created by @c newArray.
 * @param size The new size.
 * @returns A pointer to the resized array.
 * @warning GC-invoking
 */
extern Object* resizeArray(Object *array, size_t size);


//MARK: Object-orientation

/** Returns true if @c cl or a superclass of @c cl conforms to the protocol. */
extern bool conformsTo(Class *cl, EmojicodeCoin index);

/** Returns true if @c a inherits from class @c from */
extern bool inheritsFrom(Class *a, Class *from);

/** Returns true if @c object is an instance of @c cl (or of a subclass of @c cl) */
extern bool instanceof(Object *object, Class *cl);


//MARK: Garbage Collection

/**
 * Marks the object @c O pointed to by the pointer @c P to which @c of points.
 * @warning This function will modify @c P to point to an exact copy of @c O after the function call.
 */
extern void mark(Object **of);
/**
 * If the calling thread needs to be paused for the GC to run, this function will first
 * unlock @c mutex if it is not a @c NULL pointer, then block until the GC cycle is completed
 * and finally try to acquire @c mutex. Otherwise no action is performed.
 *
 * You normally do not need to call this method from method or intializer implementations.
 */
extern void pauseForGC(pthread_mutex_t *mutex);
/**
 * This function allows the GC to run even while the calling thread is not paused and working.
 * You must ensure that @c disallowGCAndPauseIfNeeded is called at an appropriate time, but it must be
 * called before the calling thread finishes.
 *
 * @warning Between the call to this function and @c disallowGCAndPauseIfNeeded you must not perform
 * any allocations or other kind of GC-invoking operations.
 */
extern void allowGC();
/**
 * See @c allowGC
 */
extern void disallowGCAndPauseIfNeeded();

//MARK: Stack

/**
 * Pushes a new stack frame with the given values.
 * @c this The object/class context.
 * @c variable The number of variables to store in the stack frame.
 */
void stackPush(Something thisContext, uint8_t variableCount, uint8_t argCount, Thread *thread);

/** Pops the current stack frame from the stack. */
void stackPop(Thread *thread);

/** Get the variable at the given index. */
Something stackGetVariable(uint8_t index, Thread *thread);
/** Set the variable at the given index. */
void stackSetVariable(uint8_t index, Something value, Thread *thread);
/** Decrements the variable at the given index. */
void stackDecrementVariable(uint8_t index, Thread *thread);
/** Increments the variable at the given index. */
void stackIncrementVariable(uint8_t index, Thread *thread);

Something* stackReserveFrame(Something thisContext, uint8_t variableCount, Thread *thread);

void stackPushReservedFrame(Thread *thread);

StackState storeStackState(Thread *thread);
void restoreStackState(StackState s, Thread *thread);

/** Returns the value on which the method was called. */
Something stackGetThisContext(Thread *);
/** Returns the object on which the method was called. */
Object* stackGetThisObject(Thread *);
/** Returns the class on which the method was called. */
__attribute__((deprecated)) Class* stackGetThisObjectClass(Thread *thread);


//MARK: Packages

typedef Something (*FunctionFunctionPointer)(Thread *thread);
typedef void (*InitializerFunctionFunctionPointer)(Thread *thread);
typedef void (*Marker)(Object *self);
typedef void (*Deinitializer)(void *value);

/** The version of a package. Must follow semantic versioning 2.0 http://semver.org */
typedef struct {
    /** The major version */
    uint16_t major;
    /** The minor version */
    uint16_t minor;
} PackageVersion;

typedef enum {
    INSTANCE_METHOD = 1, TYPE_METHOD = 2
} MethodType;

/**
 * Generates a secure random number. The integer is either generated using arc4random_uniform if available
 * or by reading from @c /dev/urandmon.
 * @param min The minimal integer (inclusive).
 * @param max The maximal integer (inclusive).
 */
extern EmojicodeInteger secureRandomNumber(EmojicodeInteger min, EmojicodeInteger max);

#endif /* EmojicodeAPI_h */
