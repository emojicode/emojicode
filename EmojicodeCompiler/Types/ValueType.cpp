//
//  ValueType.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 12/06/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "ValueType.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"

namespace EmojicodeCompiler {

void ValueType::prepareForSemanticAnalysis() {
    TypeDefinition::prepareForSemanticAnalysis();
    if (primitive_ && !instanceVariables().empty()) {
        throw CompilerError(position(), "A value type marked with ⚪️ cannot have instance variables.");
    }
    TypeDefinition::finalizeProtocols(Type(this, false));
}

void ValueType::prepareForCG() {
    TypeDefinition::prepareForCG();

    eachFunction([this](auto *function) {
        function->setVtiProvider(&vtiProvider_);
    });
}

void ValueType::addMethod(Function *method) {
    TypeDefinition::addMethod(method);
    method->package()->registerFunction(method);
}

void ValueType::addInitializer(Initializer *initializer) {
    TypeDefinition::addInitializer(initializer);
    initializer->package()->registerFunction(initializer);
}
    
void ValueType::addTypeMethod(Function *method) {
    TypeDefinition::addTypeMethod(method);
    method->package()->registerFunction(method);
}

}  // namespace EmojicodeCompiler
