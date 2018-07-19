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
#include <new>
#include <type_traits>

extern "C" int8_t* ejcAlloc(int64_t size);
extern "C" [[noreturn]] void ejcPanic(const char *message);

namespace runtime {

using Integer = int64_t;
using Symbol = uint32_t;
using Boolean = int8_t;
using Byte = int8_t;
using Real = double;
using ClassType = void*;
using Enum = int64_t;

template <typename T>
inline T* allocate(int64_t n = 1) {
    return reinterpret_cast<T*>(ejcAlloc(sizeof(T) * n));
}

template <typename Subclass>
struct Meta;

namespace util {

template <class T, std::size_t = sizeof(T)>
std::true_type is_complete_impl(T *);
std::false_type is_complete_impl(...);
template <class T>
using is_complete = decltype(is_complete_impl(std::declval<T*>()));

}  // namespace util

template <typename Subclass>
class Object {
public:
    template <typename ...Args>
    static Subclass* allocateAndInitType(Args ...args) {
        auto obj = new(allocate<Subclass>()) Subclass(args...);
        static_assert(util::is_complete<Meta<Subclass>>::value,
                      "Provide the meta type for this class with SET_META_FOR.");
        return obj;
    }
protected:
    Object() : meta_(Meta<Subclass>::value) {}
private:
    void *meta_;
};

struct ClassInfo {
    ClassInfo *superclass;
    void *dispatchTable;
};

#define OBJECT_META_NAME(package, emojitypename) package ## _class_meta_ ## emojitypename
#define SET_META_FOR(type, package, emojitypename) extern "C" runtime::ClassInfo OBJECT_META_NAME(package, emojitypename); \
template<> struct runtime::Meta<type> { constexpr static runtime::ClassInfo *const value = &OBJECT_META_NAME(package, emojitypename); };

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

struct MakeError_t {};
constexpr MakeError_t MakeError {};

template <typename Type>
class SimpleError {
public:
    SimpleError(MakeError_t, Enum errorEnumValue) : errorValue_(errorEnumValue) {}
    SimpleError(Type content) : errorValue_(-1), content_(content) {}
private:
    Enum errorValue_;
    Type content_;
};

template <typename Return, typename ...Args>
class Callable {
public:
    Return operator ()(Args... args) const {
        return function_(catpures_, args...);
    }
private:
    Return (*function_)(void*, Args...);
    void *catpures_;
};

}

#endif /* Runtime_h */
