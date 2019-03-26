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
#include "Compiler.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

Value* ASTClosure::generate(FunctionCodeGenerator *fg) const {
    if (!allocatesOnStack() && !isEscaping_) {
        auto ce = CompilerError(position(), "Using non-escaping closure as escaping value.");
        ce.addNotes(position(), "Add 🛅 after 🍇 to make closure escaping.");
        throw ce;
    }
    if (allocatesOnStack() && isEscaping_) {
        fg->compiler()->warn(position(), "Using escaping closure as non-escaping value.");
    }

    closure_->createUnspecificReification();
    fg->generator()->declarator().declareLlvmFunction(closure_.get());

    auto thisValue = capture_.capturesSelf() ? fg->thisValue()->getType() : nullptr;
    auto capture = capture_;
    capture.type = fg->generator()->typeHelper().llvmTypeForCapture(capture_, thisValue, isEscaping_);

    ClosureCodeGenerator closureGenerator(capture, closure_.get(), fg->generator(), isEscaping_);
    closureGenerator.generate();

    auto alloc = storeCapturedVariables(fg, capture);
    auto i8ptr = fg->builder().CreateBitCast(closure_->unspecificReification().function,
                                             llvm::Type::getInt8PtrTy(fg->ctx()));
    auto callable = fg->builder().CreateInsertValue(llvm::UndefValue::get(fg->typeHelper().callable()), i8ptr, 0);
    return handleResult(fg, fg->builder().CreateInsertValue(callable, alloc, 1));
}

llvm::Value* ASTClosure::createDeinit(CodeGenerator *cg, const Capture &capture) const {
    auto deinit = llvm::Function::Create(cg->typeHelper().captureDeinit(),
                                         llvm::GlobalValue::LinkageTypes::PrivateLinkage, "captureDeinit",
                                         cg->module());

    FunctionCodeGenerator fg(deinit, cg, std::make_unique<TypeContext>(closure_->typeContext()));
    fg.createEntry();

    if (isEscaping_) {
        auto captures = fg.builder().CreateBitCast(deinit->args().begin(), capture.type->getPointerTo());

        auto i = 2;
        if (capture.capturesSelf()) {
            auto ep = fg.builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++);
            fg.releaseByReference(ep, capture.self);
        }
        for (auto &capturedVar : capture.captures) {
            auto ptr = fg.builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++);
            if (capturedVar.type.isManaged()) {
                fg.releaseByReference(ptr, capturedVar.type);
            }
        }
    }

    fg.builder().CreateRetVoid();
    return deinit;
}

llvm::Value* ASTClosure::storeCapturedVariables(FunctionCodeGenerator *fg, const Capture &capture) const {
    auto captures = allocate(fg, capture.type);

    auto ep = fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, 1);
    fg->builder().CreateStore(createDeinit(fg->generator(), capture), ep);

    auto i = 2;
    if (capture.capturesSelf()) {
        if (isEscaping_) fg->retain(fg->thisValue(), capture_.self);
        auto ep = fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++);
        fg->builder().CreateStore(fg->thisValue(), ep);
    }

    if (isEscaping_) {
        for (auto &capturedVar : capture.captures) {
            Value *value;
            auto &variable = fg->scoper().getVariable(capturedVar.sourceId);

            if (capturedVar.type.isManaged() && fg->isManagedByReference(capturedVar.type)) {
                fg->retain(variable, capturedVar.type);
            }
            value = fg->builder().CreateLoad(variable);
            if (capturedVar.type.isManaged() && !fg->isManagedByReference(capturedVar.type)) {
                fg->retain(value, capturedVar.type);
            }

            fg->builder().CreateStore(value, fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++));
        }
    }
    else {
        for (auto &capturedVar : capture.captures) {
            auto &var = fg->scoper().getVariable(capturedVar.sourceId);
            fg->builder().CreateStore(var, fg->builder().CreateConstInBoundsGEP2_32(capture.type, captures, 0, i++));
        }
    }
    return fg->builder().CreateBitCast(captures, llvm::Type::getInt8PtrTy(fg->ctx()));
}

llvm::Value* ASTCallableBox::generate(FunctionCodeGenerator *fg) const {
    thunk_->createUnspecificReification();
    fg->generator()->declarator().declareLlvmFunction(thunk_.get());

    ClosureCodeGenerator closureGenerator(thunk_.get(), fg->generator());
    closureGenerator.generate();

    auto captures = allocate(fg, fg->typeHelper().callable());
    fg->builder().CreateStore(expr_->generate(fg), captures);

    auto callable = fg->builder().CreateInsertValue(llvm::UndefValue::get(fg->typeHelper().callable()),
                                                    thunk_->unspecificReification().function, 0);

    auto cp = fg->builder().CreateBitCast(captures, llvm::Type::getInt8PtrTy(fg->ctx()));
    return fg->builder().CreateInsertValue(callable, cp, 1);
}

llvm::Value* ASTCallableThunkDestination::generate(FunctionCodeGenerator *fg) const {
    return fg->thisValue();
}

}  // namespace EmojicodeCompiler
