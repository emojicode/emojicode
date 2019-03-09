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
        setVariable(i++, &llvmArg);
        llvmArg.setName(utf8(arg.name));
    }

    loadCapturedVariables(&*llvmFunction->args().begin());
}

void ClosureCodeGenerator::loadCapturedVariables(Value *value) {
    if (thunk_) {
        thisValue_ = builder().CreateLoad(builder().CreateBitCast(value, typeHelper().callable()->getPointerTo()));
        return;
    }

    Value *captures = builder().CreateBitCast(value, capture_.type->getPointerTo());

    size_t index = 2;
    if (capture_.capturesSelf()) {
        thisValue_ = builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(capture_.type, captures, 0, index++));
    }
    if (escaping_) {
        for (auto &capture : capture_.captures) {
            auto value = builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(capture_.type, captures, 0, index++));
            setVariable(capture.captureId, value);
        }
    }
    else {
        for (auto &capture : capture_.captures) {
            auto ptr = builder().CreateLoad(builder().CreateConstInBoundsGEP2_32(capture_.type, captures, 0, index++));
            scoper().getVariable(capture.captureId) = ptr;
        }
    }
}

}  // namespace EmojicodeCompiler
