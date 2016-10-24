//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <functional>
#include "ValueType.hpp"
#include "Function.hpp"

std::vector<ValueType *> ValueType::valueType_;

void ValueType::finalize() {
    for (auto f : methodList()) {
        f->setVtiAssigner(std::bind(&ValueType::nextFunctionVti, this));
    }
    for (auto f : classMethodList()) {
        f->setVtiAssigner(std::bind(&ValueType::nextFunctionVti, this));
    }
    for (auto f : initializerList()) {
        f->setVtiAssigner(std::bind(&ValueType::nextFunctionVti, this));
    }
}

int ValueType::nextFunctionVti() {
    assignedFunctionCount_++;
    return Function::nextFunctionVti();
}
