//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"
#include "ASTInitialization.hpp"

namespace EmojicodeCompiler {

Value* ASTGetVariable::instanceVariablePointer(FunctionCodeGenerator *fg, size_t index) {
    std::vector<Value *> idxList{ fg->int32(0), fg->int32(index) };
    return fg->builder().CreateGEP(fg->thisValue(), idxList);
}

Value* ASTGetVariable::generate(FunctionCodeGenerator *fg) const {
    if (inInstanceScope()) {
        auto ptr = ASTGetVariable::instanceVariablePointer(fg, varId_);
        if (reference_) {
            return ptr;
        }
        return fg->builder().CreateLoad(ptr);
    }

    auto localVariable = fg->scoper().getVariable(varId_);
    if (!localVariable.isMutable) {
        assert(!reference_);
        return localVariable.value;
    }
    if (reference_) {
        return localVariable.value;
    }
    return fg->builder().CreateLoad(localVariable.value);
}

void ASTInitableCreator::generate(FunctionCodeGenerator *fg) const {
    if (noAction_) {
        auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_);

        auto &localVariable = fg->scoper().getVariable(varId_);
        if (declare_) {
            auto alloca = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(type_));
            localVariable = LocalVariable(true, alloca);
        }

        init->setDestination(localVariable.value);
        expr_->generate(fg);
    }
    else {
        generateAssignment(fg);
    }
}

void ASTVariableDeclaration::generate(FunctionCodeGenerator *fg) const {
    auto alloca = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(type_), nullptr, utf8(varName_));
    fg->scoper().getVariable(id_) = LocalVariable(true, alloca);

    if (type_.optional()) {
        std::vector<Value *> idx { fg->int32(0), fg->int32(0) };
        fg->builder().CreateStore(fg->generator()->optionalNoValue(), fg->builder().CreateGEP(alloca, idx));
    }
}

void ASTVariableAssignmentDecl::generateAssignment(FunctionCodeGenerator *fg) const {
    llvm::Value *varPtr;
    if (declare_) {
        varPtr = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()), nullptr, utf8(varName_));
        fg->scoper().getVariable(varId_) = LocalVariable(true, varPtr);
    }
    else if (inInstanceScope()) {
        varPtr = ASTGetVariable::instanceVariablePointer(fg, varId_);
    }
    else {
        varPtr = fg->scoper().getVariable(varId_).value;
    }

    fg->builder().CreateStore(expr_->generate(fg), varPtr);
}

void ASTFrozenDeclaration::generateAssignment(FunctionCodeGenerator *fg) const {
    fg->scoper().getVariable(id_) = LocalVariable(false, expr_->generate(fg));
}

}  // namespace EmojicodeCompiler
