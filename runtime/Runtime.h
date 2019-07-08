//
//  Runtime.h
//  Emojicode
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Runtime_h
#define Runtime_h

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <new>
#include <utility>

namespace runtime {
namespace internal {
struct ControlBlock;
ControlBlock* newControlBlock();
struct Capture;
}
}

extern "C" int8_t* ejcAlloc(int64_t size);
extern "C" [[noreturn]] void ejcPanic(const char *message);

namespace runtime {

using Integer = int64_t;
using Boolean = int8_t;
using Byte = char;
using Real = double;
using Enum = int64_t;

template <typename Subclass>
struct ClassInfoFor;

namespace util {

template <class T, std::size_t = sizeof(T)>
std::true_type is_complete_impl(T *);
std::false_type is_complete_impl(...);
template <class T>
using is_complete = decltype(is_complete_impl(std::declval<T*>()));

}  // namespace util

struct ClassInfo {
    void *rtti;
    void **dispatchTable;
    void *protocolTable;
    ClassInfo *superclass;
    void (*destructor)(void*);

    template <typename Return, typename ObjectType, typename ...Args>
    Return dispatch(size_t virtualTableIndex, ObjectType *object, Args... args) const {
        return reinterpret_cast<Return(*)(ObjectType*, Args...)>(dispatchTable[virtualTableIndex])(object, args...);
    }
};

template <typename T>
class MemoryPointer {
    template <typename TA>
    friend inline MemoryPointer<TA> allocate(int64_t n);
public:
    MemoryPointer() {}
    T* get() const {
        return reinterpret_cast<T*>(pointer_ + sizeof(runtime::internal::ControlBlock *));
    }

    T& operator[](size_t index) const {
        return get()[index];
    }

    void retain();
    void release();
    
private:
    explicit MemoryPointer(int8_t *pointer) : pointer_(pointer) {}
    int8_t* pointer_ = nullptr;
};

template <typename T>
inline MemoryPointer<T> allocate(int64_t n = 1) {
    return MemoryPointer<T>(ejcAlloc(sizeof(T) * n + sizeof(runtime::internal::ControlBlock *)));
}

template <typename Subclass>
class Object {
public:
    Object& operator=(const Object&) = delete;

    template <typename ...Args>
    static Subclass* init(Args&& ...args) {
        static_assert(util::is_complete<ClassInfoFor<Subclass>>::value,
                      "Provide class info for this class with SET_INFO_FOR.");
        return new(malloc(sizeof(Subclass))) Subclass(std::forward<Args>(args)...);
    }

    internal::ControlBlock* controlBlock() const { return block_; }
    const ClassInfo* classInfo() const { return classInfo_; }

    void retain();
    void release();
protected:
    Object() : block_(internal::newControlBlock()), classInfo_(ClassInfoFor<Subclass>::value) {}
private:
    internal::ControlBlock *block_;
    const ClassInfo *classInfo_;
};

#define CLASS_INFO_NAME(package, emojitypename) package ## _class_info_ ## emojitypename
#define SET_INFO_FOR(type, package, emojitypename) extern "C" const runtime::ClassInfo CLASS_INFO_NAME(package, emojitypename); \
template<> struct runtime::ClassInfoFor<type> { constexpr static const runtime::ClassInfo *const value = &CLASS_INFO_NAME(package, emojitypename); };

struct NoValue_t {};
constexpr NoValue_t NoValue {};

template <typename Type>
class SimpleOptional {
public:
    SimpleOptional(NoValue_t) : value_(0) {}
    SimpleOptional(Type content) : value_(1), content_(content) {}

    bool operator==(NoValue_t) const {
        return !value_;
    }

    Type& operator*() {
        return content_;
    }

    const Type& operator*() const {
        return content_;
    }
private:
    Boolean value_;
    Type content_;
};

template <typename Type>
class SimpleOptional<Type*> {
public:
    SimpleOptional(NoValue_t) : pointer_(nullptr) {}
    SimpleOptional(Type* content) : pointer_(content) {}

    bool operator==(NoValue_t) const {
        return pointer_ == nullptr;
    }

    Type* operator*() const {
        return pointer_;
    }

private:
    Type *pointer_;
};

class Raiser {
public:
    template <typename T>
    void raise(T *errorObj, const char *location) {
        errorDestination_ = errorObj;
    }
private:
    void *errorDestination_;
};

#define S1(x) #x
#define S2(x) S1(x)
#define EJC_RAISE(raiser, error) raiser->raise(error, __FILE__ ":" S2(__LINE__)); return {};
#define EJC_RAISE_VOID(raiser, error) raiser->raise(error, __FILE__ ":" S2(__LINE__)); return;

template <typename Return, typename ...Args>
class Callable {
public:
    Return operator ()(Args... args) const {
        return function_(capture_, args...);
    }

    void retain() const;
    void release() const;
private:
    Return (*function_)(void*, Args...);
    runtime::internal::Capture *capture_;
};

extern "C" void ejcRetain(runtime::Object<void> *object);
extern "C" void ejcRelease(runtime::Object<void> *object);
extern "C" void ejcReleaseCapture(runtime::internal::Capture *capture);
extern "C" void ejcReleaseMemory(runtime::Object<void> *object);

template <typename Return, typename ...Args>
void Callable<Return, Args...>::retain() const {
    ejcRetain(reinterpret_cast<runtime::Object<void> *>(capture_));
}

template <typename Return, typename ...Args>
void Callable<Return, Args...>::release() const {
    ejcReleaseCapture(capture_);
}

template <typename Subclass>
void Object<Subclass>::retain() {
    ejcRetain(reinterpret_cast<runtime::Object<void> *>(this));
}

template <typename Subclass>
void Object<Subclass>::release() {
    ejcRelease(reinterpret_cast<runtime::Object<void> *>(this));
}

template <typename Type>
void MemoryPointer<Type>::retain() {
    ejcRetain(reinterpret_cast<runtime::Object<void> *>(pointer_));
}

template <typename Type>
void MemoryPointer<Type>::release() {
    ejcReleaseMemory(reinterpret_cast<runtime::Object<void> *>(pointer_));
}

}  // namespace runtime

#endif /* Runtime_h */
