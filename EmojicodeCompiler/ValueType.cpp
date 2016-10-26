//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Function.hpp"

std::vector<ValueType *> ValueType::valueType_;

void ValueType::finalize() {
    for (auto f : methodList()) {
        f->setVtiProvider(&vtiProvider_);
    }
    for (auto f : classMethodList()) {
        f->setVtiProvider(&vtiProvider_);
    }
    for (auto f : initializerList()) {
        f->setVtiProvider(&vtiProvider_);
    }
}
