//
//  CallCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CallCodeGenerator.hpp"

namespace EmojicodeCompiler {

EmojicodeInstruction CallCodeGenerator::generateArguments(const ASTArguments &args) {
    size_t argumentsSize = 0;
    for (auto &arg : args.arguments()) {
        arg->generate(fncg_);
        argumentsSize += arg->expressionType().size();
    }
    return static_cast<EmojicodeInstruction>(argumentsSize);
}

}  // namespace EmojicodeCompiler
