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
    return fg->instanceVariablePointer(id());
}

Value* AccessesAnyVariable::managementValue(FunctionCodeGenerator *fg) const {
    if (inInstanceScope()) {
        llvm::Value *objectPointer = instanceVariablePointer(fg);
        if (!fg->isManagedByReference(variableType_)) {
            objectPointer = fg->builder().CreateLoad(objectPointer);
        }
        return objectPointer;
    }

    auto &var = fg->scoper().getVariable(id());
    if (fg->isManagedByReference(variableType_)) {
        return getReferenceWithPromotion(var, fg);
    }
    return var.isMutable ? fg->builder().CreateLoad(var.value) : var.value;
}

Value* AccessesAnyVariable::getReferenceWithPromotion(LocalVariable &variable, FunctionCodeGenerator *fg) const {
    if (!variable.isMutable) {
        auto alloca = fg->createEntryAlloca(variable.value->getType());
        fg->builder().CreateStore(variable.value, alloca);
        variable = LocalVariable(true, alloca);
    }
    return variable.value;
}

void AccessesAnyVariable::release(FunctionCodeGenerator *fg) const {
    fg->release(managementValue(fg), variableType_);
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
    if (reference_) {
        return getReferenceWithPromotion(localVariable, fg);
    }

    if (!returned_ && !isTemporary() && expressionType().isManaged()) {
        auto retainValue = managementValue(fg);
        fg->retain(retainValue, expressionType());
        handleResult(fg, retainValue);
    }
    return localVariable.isMutable ? fg->builder().CreateLoad(localVariable.value) : localVariable.value;
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
        fg->builder().CreateStore(fg->buildSimpleOptionalWithoutValue(type_->type()), alloca);
    }
}

void ASTVariableAssignment::generateAssignment(FunctionCodeGenerator *fg) const {
    if (wasInitialized_ && variableType().isManaged()) {
        release(fg);
    }
    auto val = expr_->generate(fg);
    auto variablePtr = variablePointer(fg);
    fg->builder().CreateStore(val, variablePtr);
}

void ASTConstantVariable::generateAssignment(FunctionCodeGenerator *fg) const {
    fg->scoper().getVariable(id()) = LocalVariable(false, expr_->generate(fg));
}

}  // namespace EmojicodeCompiler
