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

struct Class {
    Class() {}
    explicit Class(void (*mark)(Object *)) : instanceVariableRecords(nullptr), mark(mark), size(0), valueSize(0) {}

    /** Returns true if @c a inherits from class @c from */
    bool inheritsFrom(Class *from) const;

    Function **methodsVtable;
    Function **initializersVtable;

    ProtocolDispatchTable protocolTable;

    /** The class’s superclass */
    struct Class *superclass;

    ObjectVariableRecord *instanceVariableRecords;
    unsigned int instanceVariableRecordsCount;

    /** Marker FunctionPointer for GC */
    void (*mark)(Object *self);

    size_t size;
    size_t valueSize;
};

#endif /* Class_hpp */
