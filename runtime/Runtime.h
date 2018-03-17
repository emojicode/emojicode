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
#include <type_traits>

extern "C" int8_t* ejcAlloc(int64_t size);

namespace runtime {

using Integer = int64_t;
using Symbol = uint32_t;
using Boolean = int8_t;
using ClassType = void*;

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
    static Subclass* allocateAndInitType() {
        auto obj = allocate<Subclass>();
        static_assert(util::is_complete<Meta<Subclass>>::value,
                      "Provide the meta type for this class with SET_META_FOR.");
        obj->meta_ = Meta<Subclass>::value;
        return obj;
    }
protected:
    Object() = default;
private:
    void *meta_;
};

struct NoValue_t {};
constexpr NoValue_t NoValue {};

template <typename Type>
class SimpleOptional {
public:
    SimpleOptional(NoValue_t) : value_(0) {}
    SimpleOptional(Type content) : value_(1), content_(content) {}
private:
    Boolean value_;
    Type content_;
};

#define OBJECT_META_NAME(package, emojitypename) package ## _class_meta_ ## emojitypename
#define SET_META_FOR(type, package, emojitypename) extern "C" char OBJECT_META_NAME(package, emojitypename)[]; \
template<> struct runtime::Meta<type> { constexpr static void *const value = &OBJECT_META_NAME(package, emojitypename); };

}

#endif /* Runtime_h */
