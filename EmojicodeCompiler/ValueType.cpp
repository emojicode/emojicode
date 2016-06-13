//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "Function.hpp"

void ValueType::addMethod(Method *method) {
    TypeDefinitionFunctional::addMethod(method);
    Function::addFunction(method);
}

void ValueType::addInitializer(Initializer *method) {
    TypeDefinitionFunctional::addInitializer(method);
    Function::addFunction(method);
}

void ValueType::addClassMethod(ClassMethod *method) {
    TypeDefinitionFunctional::addClassMethod(method);
    Function::addFunction(method);
}