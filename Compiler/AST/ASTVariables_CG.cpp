//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "ASTInitialization.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* AccessesAnyVariable::instanceVariablePointer(FunctionCodeGenerator *fg) const {
    assert(inInstanceScope());
    std::vector<Value *> idxList{ fg->int32(0), fg->int32(id()) };
    return fg->builder().CreateGEP(fg->thisValue(), idxList);
}

Value* ASTGetVariable::generate(FunctionCodeGenerator *fg) const {
    if (inInstanceScope()) {
        auto ptr = instanceVariablePointer(fg);
        if (reference_) {
            return ptr;
        }
        return fg->builder().CreateLoad(ptr);
    }

    auto &localVariable = fg->scoper().getVariable(id());
    if (!localVariable.isMutable) {
        if (reference_) {
            auto alloca = fg->builder().CreateAlloca(localVariable.value->getType());
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
        auto varPtr = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(expr_->expressionType()), nullptr,
                                            utf8(name()));
        fg->scoper().getVariable(id()) = LocalVariable(true, varPtr);
        return varPtr;
    }

    if (inInstanceScope()) {
        return instanceVariablePointer(fg);
    }

    return fg->scoper().getVariable(id()).value;
}

void ASTVariableDeclaration::generate(FunctionCodeGenerator *fg) const {
    auto alloca = fg->builder().CreateAlloca(fg->typeHelper().llvmTypeFor(type_), nullptr, utf8(varName_));
    fg->scoper().getVariable(id_) = LocalVariable(true, alloca);

    if (type_.type() == TypeType::Optional) {
        std::vector<Value *> idx { fg->int32(0), fg->int32(0) };
        fg->builder().CreateStore(fg->generator()->optionalNoValue(), fg->builder().CreateGEP(alloca, idx));
    }
}

void ASTVariableAssignment::generateAssignment(FunctionCodeGenerator *fg) const {
    fg->builder().CreateStore(expr_->generate(fg), variablePointer(fg));
}

void ASTConstantVariable::generateAssignment(FunctionCodeGenerator *fg) const {
    fg->scoper().getVariable(id()) = LocalVariable(false, expr_->generate(fg));
}

}  // namespace EmojicodeCompiler
