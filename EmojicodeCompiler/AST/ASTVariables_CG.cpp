//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTGetVariable::instanceVariablePointer(FunctionCodeGenerator *fncg, size_t index) {
    std::vector<Value *> idxList{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), index),
    };
    return fncg->builder().CreateGEP(fncg->thisValue(), idxList);
}

Value* ASTGetVariable::generate(FunctionCodeGenerator *fncg) const {
    if (inInstanceScope()) {
        auto ptr = ASTGetVariable::instanceVariablePointer(fncg, varId_);
        if (reference_) {
            return ptr;
        }
        return fncg->builder().CreateLoad(ptr);
    }

    auto localVariable = fncg->scoper().getVariable(varId_);
    if (!localVariable.isMutable) {
        assert(!reference_);
        return localVariable.value;
    }
    if (reference_) {
        return localVariable.value;
    }
    return fncg->builder().CreateLoad(localVariable.value);
}

Value* ASTInitGetVariable::generate(FunctionCodeGenerator *fncg) const {
    if (declare_) {
        auto alloca = fncg->builder().CreateAlloca(fncg->typeHelper().llvmTypeFor(type_));
        fncg->scoper().getVariable(varId_) = LocalVariable(true, alloca);
    }
    return ASTGetVariable::generate(fncg);
}

void ASTInitableCreator::generate(FunctionCodeGenerator *fncg) const {
    if (noAction_) {
        expr_->generate(fncg);
    }
    else {
        generateAssignment(fncg);
    }
}

void ASTVariableDeclaration::generate(FunctionCodeGenerator *fncg) const {
    auto alloca = fncg->builder().CreateAlloca(fncg->typeHelper().llvmTypeFor(type_), nullptr, utf8(varName_));
    fncg->scoper().getVariable(id_) = LocalVariable(true, alloca);

    if (type_.optional()) {
        std::vector<Value *> idx {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), 0),
        };
        fncg->builder().CreateStore(fncg->generator()->optionalNoValue(), fncg->builder().CreateGEP(alloca, idx));
    }
}

void ASTVariableAssignmentDecl::generateAssignment(FunctionCodeGenerator *fncg) const {
    llvm::Value *varPtr;
    if (declare_) {
        varPtr = fncg->builder().CreateAlloca(fncg->typeHelper().llvmTypeFor(expr_->expressionType()), nullptr, utf8(varName_));
        fncg->scoper().getVariable(varId_) = LocalVariable(true, varPtr);
    }
    else if (inInstanceScope()) {
        varPtr = ASTGetVariable::instanceVariablePointer(fncg, varId_);
    }
    else {
        varPtr = fncg->scoper().getVariable(varId_).value;
    }

    fncg->builder().CreateStore(expr_->generate(fncg), varPtr);
}

void ASTFrozenDeclaration::generateAssignment(FunctionCodeGenerator *fncg) const {
    fncg->scoper().getVariable(id_) = LocalVariable(false, expr_->generate(fncg));
}

}  // namespace EmojicodeCompiler
