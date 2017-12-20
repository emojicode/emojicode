//
//  ASTClosure_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Generation/ClosureCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTClosure::generate(FunctionCodeGenerator *fg) const {
    fg->generator()->declareLlvmFunction(closure_.get());

    auto closureGenerator = ClosureCodeGenerator(captures_, closure_.get(), fg->generator());
    closureGenerator.generate();

    auto capturesType = fg->generator()->typeHelper().llvmTypeForClosureCaptures(captures_);
    auto alloc = fg->builder().CreateCall(fg->generator()->runTimeNew(), fg->sizeOf(capturesType));
    auto captures = fg->builder().CreateBitCast(alloc, capturesType->getPointerTo());


    auto i = 0;
    for (auto &capture : captures_) {
        llvm::Value *value;
        auto &variable = fg->scoper().getVariable(capture.sourceId);
        if (variable.isMutable) {
            value = fg->builder().CreateLoad(variable.value);
        }
        else {
            value = variable.value;
        }

        fg->builder().CreateStore(value, fg->builder().CreateConstGEP2_32(capturesType, captures, 1, i++));
    }

    auto *structType = llvm::dyn_cast<llvm::StructType>(fg->generator()->typeHelper().llvmTypeFor(expressionType()));


    auto callable = fg->builder().CreateInsertValue(llvm::UndefValue::get(structType), closure_->llvmFunction(), 0);
    return fg->builder().CreateInsertValue(callable, alloc, 1);
}

}  // namespace EmojicodeCompiler
