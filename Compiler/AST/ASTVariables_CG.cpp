//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "ASTInitialization.hpp"
#include "ASTMemory.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Types/ValueType.hpp"

namespace EmojicodeCompiler {

Value* AccessesAnyVariable::instanceVariablePointer(FunctionCodeGenerator *fg) const {
    assert(inInstanceScope());
    auto type = llvm::cast<llvm::PointerType>(fg->thisValue()->getType())->getElementType();
    return fg->builder().CreateConstInBoundsGEP2_32(type, fg->thisValue(), 0, id());
}

Value* AccessesAnyVariable::managementValue(FunctionCodeGenerator *fg) const {
    if (inInstanceScope()) {
        llvm::Value *objectPointer;
        objectPointer = instanceVariablePointer(fg);
        if (!fg->isManagedByReference(variableType_)) {
            objectPointer = fg->builder().CreateLoad(objectPointer);
        }
        return objectPointer;
    }

    auto &var = fg->scoper().getVariable(id());
    if (fg->isManagedByReference(variableType_)) {
        if (var.isMutable) return var.value;
        auto alloc = fg->createEntryAlloca(var.value->getType());
        fg->builder().CreateStore(var.value, alloc);
        return alloc;
    }
    return var.isMutable ? fg->builder().CreateLoad(var.value) : var.value;
}

void AccessesAnyVariable::release(FunctionCodeGenerator *fg) const {
    fg->release(managementValue(fg), variableType_, false);
}

Value* ASTGetVariable::generate(FunctionCodeGenerator *fg) const {
    if (inInstanceScope()) {
        auto ptr = instanceVariablePointer(fg);
        if (reference_) {
            return ptr;
        }
        auto val = fg->builder().CreateLoad(ptr);
        if (expressionType().isManaged()) {
            auto retainValue = fg->isManagedByReference(expressionType()) ? ptr : val;
            fg->retain(retainValue, expressionType());
            handleResult(fg, retainValue, true);
        }
        return val;
    }

    auto &localVariable = fg->scoper().getVariable(id());
    if (!localVariable.isMutable) {
        if (reference_) {
            auto alloca = fg->createEntryAlloca(localVariable.value->getType());
            fg->builder().CreateStore(localVariable.value, alloca);
            localVariable = LocalVariable(true, alloca);
            return localVariable.value;
        }
        return localVariable.value;
    }
    if (reference_) {
        return localVariable.value;
    }
    return fg->builder().CreateLoad(localVariable.value);
}

void ASTVariableInit::generate(FunctionCodeGenerator *fg) const {
    if (auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_)) {
        if (init->initType() == ASTInitialization::InitType::ValueType) {
            init->setDestination(variablePointer(fg));
            expr_->generate(fg);
            return;
        }
    }

    generateAssignment(fg);
}

llvm::Value* ASTVariableInit::variablePointer(EmojicodeCompiler::FunctionCodeGenerator *fg) const {
    if (declare_) {
        auto varPtr = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()), utf8(name()));
        fg->scoper().getVariable(id()) = LocalVariable(true, varPtr);
        return varPtr;
    }

    if (inInstanceScope()) {
        return instanceVariablePointer(fg);
    }

    auto &var = fg->scoper().getVariable(id());
    assert(var.isMutable);
    return var.value;
}

void ASTVariableDeclaration::generate(FunctionCodeGenerator *fg) const {
    auto type = fg->typeHelper().llvmTypeFor(type_->type());
    auto alloca = fg->createEntryAlloca(type, utf8(varName_));
    fg->scoper().getVariable(id_) = LocalVariable(true, alloca);

    if (type_->type().type() == TypeType::Optional) {
        fg->builder().CreateStore(fg->generator()->optionalNoValue(),
                                  fg->builder().CreateConstInBoundsGEP2_32(type, alloca, 0, 0));
    }
}

void ASTVariableAssignment::generateAssignment(FunctionCodeGenerator *fg) const {
    if (wasInitialized_ && variableType().isManaged()) {
        release(fg);
    }
    auto val = expr_->generate(fg);
    auto variablePtr = variablePointer(fg);
    fg->builder().CreateStore(val, variablePtr);
    if (variableType().isManaged()) {
        fg->retain(fg->isManagedByReference(expr_->expressionType()) ? variablePtr : val, expr_->expressionType());
    }
}

void ASTConstantVariable::generateAssignment(FunctionCodeGenerator *fg) const {
    fg->scoper().getVariable(id()) = LocalVariable(false, expr_->generate(fg));
}

}  // namespace EmojicodeCompiler
