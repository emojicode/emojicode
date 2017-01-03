//
//  Protocol.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Protocol.hpp"
#include "Function.hpp"

uint_fast16_t Protocol::nextIndex = 0;

Protocol::Protocol(EmojicodeString name, Package *pkg, SourcePosition p, const EmojicodeString &string)
    : TypeDefinitionFunctional(name, pkg, p, string) {
    if (nextIndex == UINT16_MAX) {
        throw CompilerErrorException(p, "You exceeded the limit of 65,536 protocols.");
    }
    index = nextIndex++;
}

void Protocol::addMethod(Function *method) {
    method->setLinkingTableIndex(1);
    method->setVti(static_cast<int>(methodList_.size()));
    TypeDefinitionFunctional::addMethod(method);
}

bool Protocol::canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) {
    return resolutionConstraint == this;
}
