//
//  Class.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeAPI.h"
#include "Emojicode.h"

bool inheritsFrom(Class *a, Class *from){
    for(; a != NULL; a = a->superclass){
        if(a == from) {
            return true;
        }
    }
    return false;
}

bool conformsTo(Class *a, EmojicodeCoin protocolIndex){
    if(a->protocolsTable == NULL || protocolIndex < a->protocolsOffset || protocolIndex > a->protocolsMaxIndex){
        return false;
    }
    return a->protocolsTable[protocolIndex - a->protocolsOffset] != NULL;
}
