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
int ValueType::nextId = 3;

void ValueType::finalize() {
    TypeDefinitionFunctional::finalize();

    if (primitive_ && instanceVariables().size() > 0) {
        throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
    }

    for (auto f : methodList()) {
        f->setVtiProvider(&vtiProvider_);
        f->package()->registerFunction(f);
    }
    for (auto f : typeMethodList()) {
        f->setVtiProvider(&vtiProvider_);
        f->package()->registerFunction(f);
    }
    for (auto f : initializerList()) {
        f->setVtiProvider(&vtiProvider_);
        f->package()->registerFunction(f);
    }
}
