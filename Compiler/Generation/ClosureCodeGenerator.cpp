//
//  ClosureCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 21/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ClosureCodeGenerator.hpp"
#include "Functions/Function.hpp"
#include <llvm/IR/Function.h>

namespace EmojicodeCompiler {

void ClosureCodeGenerator::declareArguments(llvm::Function *llvmFunction) {
    unsigned int i = 0;
    auto it = llvmFunction->args().begin();
    (it++)->setName("captures");
    for (auto &arg : function()->parameters()) {
        auto &llvmArg = *(it++);
        scoper().getVariable(i++) = LocalVariable(false, &llvmArg);
        llvmArg.setName(utf8(arg.name));
    }

    loadCapturedVariables(&*llvmFunction->args().begin());
}

void ClosureCodeGenerator::loadCapturedVariables(Value *value) {
    Value *captures = builder().CreateBitCast(value, capture_.type->getPointerTo());

    size_t index = 1;
    if (capture_.capturesSelf()) {
        thisValue_ = builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(capture_.type, captures, 0, index++));
    }
    for (auto &capture : capture_.captures) {
        auto *value = builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(capture_.type, captures, 0, index++));
        scoper().getVariable(capture.captureId) = LocalVariable(false, value);
    }
}

}  // namespace EmojicodeCompiler
