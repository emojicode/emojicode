//
//  CommonTypeFinder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 28/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CommonTypeFinder.hpp"
#include "TypeContext.hpp"

void CommonTypeFinder::addType(Type t, TypeContext typeContext) {
    if (!firstTypeFound) {
        commonType = t;
        firstTypeFound = true;
    }
    else if (!t.compatibleTo(commonType, typeContext)) {
        if (commonType.compatibleTo(t, typeContext)) {
            commonType = t;
        }
        else if (t.type() == TypeContent::Class && commonType.type() == TypeContent::Class) {
            commonType = typeSomeobject;
        }
        else {
            commonType = typeSomething;
        }
    }
}

Type CommonTypeFinder::getCommonType(const Token &warningToken) const {
    if (!firstTypeFound) {
        compilerWarning(warningToken, "Type is ambigious without more context.");
    }
    return commonType;
}
