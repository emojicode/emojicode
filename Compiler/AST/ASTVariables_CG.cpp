//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "ASTMemory.hpp"
#include "ASTVariables.hpp"
#include "Generation/Declarator.hpp"
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

    auto var = fg->scoper().getVariable(id());
    return fg->isManagedByReference(variableType_) ? var : fg->builder().CreateLoad(var);
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
            fg->retain(fg->isManagedByReference(expressionType()) ? ptr : val, expressionType());
            handleResult(fg, val, ptr);
        }
        return val;
    }

    auto &localVariable = fg->scoper().getVariable(id());
    if (reference_) {
        return localVariable;
    }

    auto val = fg->builder().CreateLoad(localVariable);
    if (!returned_ && !isTemporary() && expressionType().isManaged()) {
        fg->retain(fg->isManagedByReference(expressionType()) ? localVariable : val, expressionType());
        handleResult(fg, val, localVariable);
    }
    return val;
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
        fg->scoper().getVariable(id()) = varPtr;
        return varPtr;
    }

    if (inInstanceScope()) {
        return instanceVariablePointer(fg);
    }

    return fg->scoper().getVariable(id());
}

void ASTVariableDeclaration::generate(FunctionCodeGenerator *fg) const {
    auto type = fg->typeHelper().llvmTypeFor(type_->type());
    auto alloca = fg->createEntryAlloca(type, utf8(varName_));
    fg->scoper().getVariable(id_) = alloca;

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
    fg->setVariable(id(), expr_->generate(fg), utf8(name()));
}

Value* ASTIsOnlyReference::generate(FunctionCodeGenerator *fg) const {
    Value *val;
    if (inInstanceScope()) {
        val = fg->builder().CreateLoad(instanceVariablePointer(fg));
    }
    else {
        auto &localVariable = fg->scoper().getVariable(id());
        val = fg->builder().CreateLoad(localVariable);
    }
    auto ptr = fg->builder().CreateBitCast(val, llvm::Type::getInt8PtrTy(fg->generator()->context()));
    return fg->builder().CreateCall(fg->generator()->declarator().isOnlyReference(), ptr);
}

}  // namespace EmojicodeCompiler
