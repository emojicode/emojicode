//
//  RetainedObjectPointer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 26/05/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef RetainedObjectPointer_hpp
#define RetainedObjectPointer_hpp

#include <cstddef>

namespace Emojicode {

class Thread;
struct Object;

class RetainedObjectPointer {
public:
    /// Constructs a RetainedObjectPointer that will always evaluate to @c nullptr.
    RetainedObjectPointer(std::nullptr_t n) : retainListPointer_(&nullobject) {}
    friend Thread;
    /// Accesses the object this pointer points to.
    Object* operator->() const {
        return *retainListPointer_;
    }
    /// Returns an ordinary pointer to the retained object.
    /// @warning This pointer is only valid until the next garbage collector cycle. This does, of course, not pose a
    /// problem if you store the pointer into another object that can be reached by the garbage collector or if you
    /// return it.
    Object* unretainedPointer() const {
        return *retainListPointer_;
    }
    /// Sets the pointer to point and retain to @c object.
    void operator=(Object *object) {
        *retainListPointer_ = object;
    }
    void operator=(std::nullptr_t) = delete;
private:
    static Object *nullobject;
    explicit RetainedObjectPointer(Object **retainListPointer) : retainListPointer_(retainListPointer) {}
    Object **retainListPointer_;
};

}  // namespace Emojicode

#endif /* RetainedObjectPointer_hpp */
