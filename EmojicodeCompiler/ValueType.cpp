//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Function.hpp"

namespace EmojicodeCompiler {

std::vector<ValueType *> ValueType::valueTypes_;

void ValueType::finalize() {
    TypeDefinitionFunctional::finalize();

    if (primitive_ && !instanceVariables().empty()) {
        throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
    }

    for (auto f : methodList()) {
        f->setVtiProvider(&vtiProvider_);
    }
    for (auto f : typeMethodList()) {
        f->setVtiProvider(&vtiProvider_);
    }
    for (auto f : initializerList()) {
        f->setVtiProvider(&vtiProvider_);
    }

    TypeDefinitionFunctional::finalizeProtocols(Type(this, false), &vtiProvider_);
}

}  // namespace EmojicodeCompiler
