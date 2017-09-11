//
//  ClosureCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ClosureCodeGenerator.hpp"
#include "../Functions/Function.hpp"

namespace EmojicodeCompiler {

void ClosureCodeGenerator::declareArguments(llvm::Function *llvmFunction) {
    unsigned int i = 0;
    auto it = llvmFunction->args().begin();
    (it++)->setName("captures");
    for (auto arg : function()->arguments) {
        auto &llvmArg = *(it++);
        scoper().getVariable(i++) = LocalVariable(false, &llvmArg);
        llvmArg.setName(utf8(arg.variableName));
    }

    auto type = typeHelper().llvmTypeForClosureCaptures(captures_);
    llvm::Value *captures = builder().CreateBitCast(llvmFunction->args().begin(), type->getPointerTo());

    for (size_t i = 0; i < captures_.size(); i++) {
        auto *value = builder().CreateLoad(builder().CreateConstGEP2_32(type, captures, 0, i));
        scoper().getVariable(captures_[i].captureId) = LocalVariable(false, value);
    }
}

}  // namespace EmojicodeCompiler
