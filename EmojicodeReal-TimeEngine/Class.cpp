//
//  Class.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Class.hpp"

bool Class::inheritsFrom(Class *from) const {
    for (const Class *a = this; a != nullptr; a = a->superclass) {
        if (a == from) {
            return true;
        }
    }
    return false;
}

bool Class::conformsTo(int protocolIndex) const {
    if (protocolsTable == nullptr || protocolIndex < protocolsOffset || protocolIndex > protocolsMaxIndex) {
        return false;
    }
    return protocolsTable[protocolIndex - protocolsOffset] != nullptr;
}
