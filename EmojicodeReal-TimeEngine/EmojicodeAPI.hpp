//
//  EmojicodeAPI.h
//  Emojicode
//
//  Created by Theo Weidmann on 12.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#ifndef EmojicodeAPI_h
#define EmojicodeAPI_h

#include "../EmojicodeShared.h"
#include <cstddef>

namespace Emojicode {

typedef unsigned char Byte;
typedef int_fast64_t EmojicodeInteger;

struct Class;
struct Object;
class Thread;

extern Class *CL_STRING;
extern Class *CL_LIST;
extern Class *CL_DATA;
extern Class *CL_DICTIONARY;
extern Class *CL_CLOSURE;
extern Class *CL_ARRAY;

/// A one-Emojicode-word large value.
union Value {
    /// Returns an undefined Value
    Value() {}
    Value(bool raw) : raw(raw) {}
    Value(EmojicodeInteger raw) : raw(raw) {}
    Value(EmojicodeChar raw) : character(raw) {}
    Value(Object *object) : object(object) {}
    Value(Value *value) : value(value) {}
    Value(Class *klass) : klass(klass) {}
    Value(double doubl) : doubl(doubl) {}
    EmojicodeInteger raw;
    EmojicodeChar character;
    double doubl;
    Object *object;
    Class *klass;
    Value *value;
};

struct Object {
    union {
        /// The class of the object
        Class *klass;
        /// Used by the Garbage Collector, do not change!
        Object *newLocation;
    };
    /** The size of this object: the size of the Object struct and the value area. */
    size_t size;

    template <typename T>
    inline T* val() {
#ifdef DEBUG 
        if (size == 0 && klass == nullptr) throw;
#endif
        return reinterpret_cast<T*>(this + 1);
    }

    inline Value* variableDestination(EmojicodeInstruction index) {
#ifdef DEBUG
        if (size == 0 && klass == nullptr) throw;
#endif
        return reinterpret_cast<Value *>(reinterpret_cast<Byte *>(this) + sizeof(Object) + sizeof(Value) * index);
    }
};

const int kBoxValueSize = 4;

/// Convenience wrapper for box storage.
/// Used to store a value when the type of the value is not known at compile time.
struct Box {
    Box(EmojicodeInteger type, Value value1) : type(type), value1(value1) {}
    Box() {}
    /// The type of the value stored in this box.
    Value type;
    Value value1;
    Value value2;
    Value value3;
    /// Returns true if this Box contains Nothingness
    bool isNothingness() const { return type.raw == T_NOTHINGNESS; }
    /// Makes this Box contain Nothingness
    void makeNothingness() { type.raw = T_NOTHINGNESS; }
    void copySingleValue(EmojicodeInteger type, Value value) { this->type = type; value1 = value; }
    void copy(Value *value) {  *this = *reinterpret_cast<Box *>(value); }
    [[deprecated]] void copyTo(Value *value) const { *reinterpret_cast<Box *>(value) = *this; }
};

// MARK: Callables

/** You can use this function to call a callable object. It’s internally Garbage-Collector safe. */
extern void executeCallableExtern(Object *callable, Value *args, size_t argsSize, Thread *thread);


// MARK: Primitive Objects

/**
 * Allocates a new object for the given class.
 * The @c value field will point to a value area as large as specified for the given class.
 * @param klass The class of the object.
 * @warning GC-invoking
 */
extern Object* newObject(Class *klass);

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
extern Object* resizeArray(Object *array, size_t size, Thread *thread);


// MARK: Garbage Collection

/**
 * Marks the object @c O pointed to by the pointer @c P to which @c of points.
 * @warning This function will modify @c P to point to an exact copy of @c O after the function call.
 */
extern void mark(Object **of);
/**
 * If the calling thread needs to be paused for the GC to run, this function will first
 * unlock @c mutex if it is not a @c nullptr pointer, then block until the GC cycle is completed
 * and finally try to acquire @c mutex. Otherwise no action is performed.
 *
 * You normally do not need to call this method from method or intializer implementations.
 */
extern void pauseForGC();
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

typedef void (*FunctionFunctionPointer)(Thread *thread);
typedef void (*Marker)(Object *self);

#undef major
#undef minor

/// The version of a package. Must follow semantic versioning 2.0 http://semver.org
struct PackageVersion {
    PackageVersion(uint16_t major, uint16_t minor) noexcept : major(major), minor(minor) {}
    /** The major version */
    uint16_t major;
    /** The minor version */
    uint16_t minor;
};

}

#define LinkingTable Emojicode::FunctionFunctionPointer linkingTable[] =

#endif /* EmojicodeAPI_h */
