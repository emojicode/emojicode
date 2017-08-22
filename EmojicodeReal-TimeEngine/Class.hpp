//
//  Class.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Class_hpp
#define Class_hpp

#include "Engine.hpp"

namespace Emojicode {

struct Class {
    Class() {}
    explicit Class(void (*mark)(Object *)) noexcept
    : instanceVariableRecords(nullptr), mark(mark), size(0), valueSize(0) {}

    /// Returns true if @c a inherits from class @c from
    bool inheritsFrom(Class *from) const;

    Function **methodsVtable;
    Function **initializersVtable;

    ProtocolDispatchTable protocolTable;

    /** The class’s superclass */
    Class *superclass;

    ObjectVariableRecord *instanceVariableRecords;
    unsigned int instanceVariableRecordsCount;

    /** Marker FunctionPointer for GC */
    void (*mark)(Object *self) = nullptr;
    void (*deinit)(Object *self);

    /// The exact size of the object when allocated.
    /// Equivalent to @c alignSize(valueSize + size for instance variables + sizeof(Object))
    size_t size;
    size_t valueSize;
};

}

#endif /* Class_hpp */
