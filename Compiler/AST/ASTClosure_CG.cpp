//
//  ASTClosure_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Generation/ClosureCodeGenerator.hpp"
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

Value* ASTClosure::generate(FunctionCodeGenerator *fg) const {
    closure_->createUnspecificReification();
    fg->generator()->declarator().declareLlvmFunction(closure_.get());

    auto thisValue = capture_.captureSelf ? fg->thisValue()->getType() : nullptr;
    auto capture = capture_;
    capture.type = fg->generator()->typeHelper().llvmTypeForCapture(capture_, thisValue);

    auto closureGenerator = ClosureCodeGenerator(capture, closure_.get(), fg->generator());
    closureGenerator.generate();

    llvm::CallInst *alloc = storeCapturedVariables(fg, capture);

    auto *structType = llvm::dyn_cast<llvm::StructType>(fg->generator()->typeHelper().llvmTypeFor(expressionType()));

    auto callable = fg->builder().CreateInsertValue(llvm::UndefValue::get(structType),
                                                    closure_->unspecificReification().function, 0);
    return fg->builder().CreateInsertValue(callable, alloc, 1);
}

llvm::CallInst *ASTClosure::storeCapturedVariables(FunctionCodeGenerator *fg, const Capture &capture) const {
    auto alloc = fg->builder().CreateCall(fg->generator()->declarator().runTimeNew(), fg->sizeOf(capture.type));
    auto captures = fg->builder().CreateBitCast(alloc, capture.type->getPointerTo());

    auto i = 0;
    if (capture.captureSelf) {
        fg->builder().CreateStore(fg->thisValue(), fg->builder().CreateConstGEP2_32(capture.type, captures, 0, i++));
    }
    for (auto &capturedVar : capture.captures) {
        Value *value;
        auto &variable = fg->scoper().getVariable(capturedVar.sourceId);
        if (variable.isMutable) {
            value = fg->builder().CreateLoad(variable.value);
        }
        else {
            value = variable.value;
        }

        fg->builder().CreateStore(value, fg->builder().CreateConstGEP2_32(capture.type, captures, 0, i++));
    }
    return alloc;
}

}  // namespace EmojicodeCompiler
