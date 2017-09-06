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

}  // namespace EmojicodeCompiler
