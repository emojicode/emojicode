//
//  ASTClosure_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTClosure.hpp"
#include "Generation/ClosureCodeGenerator.hpp"
#include "Generation/Declarator.hpp"
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

Value* ASTClosure::generate(FunctionCodeGenerator *fg) const {
    closure_->createUnspecificReification();
    fg->generator()->declarator().declareLlvmFunction(closure_.get());

    auto thisValue = capture_.capturesSelf() ? fg->thisValue()->getType() : nullptr;
    auto capture = capture_;
    capture.type = fg->generator()->typeHelper().llvmTypeForCapture(capture_, thisValue);

    auto closureGenerator = ClosureCodeGenerator(capture, closure_.get(), fg->generator());
    closureGenerator.generate();

    auto alloc = storeCapturedVariables(fg, capture);

    auto *structType = llvm::dyn_cast<llvm::StructType>(fg->generator()->typeHelper().llvmTypeFor(expressionType()));

    auto callable = fg->builder().CreateInsertValue(llvm::UndefValue::get(structType),
                                                    closure_->unspecificReification().function, 0);
    return fg->builder().CreateInsertValue(callable, alloc, 1);
}

llvm::Value* ASTClosure::storeCapturedVariables(FunctionCodeGenerator *fg, const Capture &capture) const {
    auto captures = allocate(fg, capture.type);

    auto i = 1;
    if (capture.capturesSelf()) {
        fg->retain(fg->thisValue(), capture_.self);
        auto ep = fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++);
        fg->builder().CreateStore(fg->thisValue(), ep);
    }
    for (auto &capturedVar : capture.captures) {
        Value *value;
        auto &variable = fg->scoper().getVariable(capturedVar.sourceId);

        if (variable.isMutable) {
            if (capturedVar.type.isManaged() && fg->isManagedByReference(capturedVar.type)) {
                fg->retain(variable.value, capturedVar.type);
            }
            value = fg->builder().CreateLoad(variable.value);
            if (capturedVar.type.isManaged() && !fg->isManagedByReference(capturedVar.type)) {
                fg->retain(value, capturedVar.type);
            }
        }
        else {
            value = variable.value;
            if (capturedVar.type.isManaged()) {
                assert(!fg->isManagedByReference(capturedVar.type));
                fg->retain(value, capturedVar.type);
            }
        }

        fg->builder().CreateStore(value, fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++));
    }
    return fg->builder().CreateBitCast(captures, llvm::Type::getInt8PtrTy(fg->generator()->context()));
}

}  // namespace EmojicodeCompiler
