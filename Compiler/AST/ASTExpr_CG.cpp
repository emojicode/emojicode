//
//  ASTExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "ASTTypeExpr.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

llvm::Value* ASTExpr::handleResult(FunctionCodeGenerator *fg, llvm::Value *result, llvm::Value *vtReference) const {
    if (producesTemporaryObject()) {
        if (fg->isManagedByReference(expressionType())) {
            if (vtReference == nullptr) {
                assert(result != nullptr);
                auto temp = fg->createEntryAlloca(result->getType());
                fg->builder().CreateStore(result, temp);
                fg->addTemporaryObject(temp, expressionType());
            }
            else {
                fg->addTemporaryObject(vtReference, expressionType());
            }
        }
        else {
            assert(result != nullptr);
            fg->addTemporaryObject(result, expressionType());
        }
    }
    return result;
}

bool ASTExpr::producesTemporaryObject() const {
    return isTemporary_ && expressionType().isManaged() && !expressionType().isReference();
}

Value* ASTTypeAsValue::generate(FunctionCodeGenerator *fg) const {
    if (type_->type().type() == TypeType::Class) {
        return type_->type().klass()->classInfo();
    }
    return llvm::UndefValue::get(fg->typeHelper().llvmTypeFor(Type(MakeTypeAsValue, type_->type())));
}

Value* ASTSizeOf::generate(FunctionCodeGenerator *fg) const {
    return fg->sizeOf(fg->typeHelper().llvmTypeFor(type_->type()));
}

Value* ASTConditionalAssignment::generate(FunctionCodeGenerator *fg) const {
    auto optional = expr_->generate(fg);

    if (expr_->expressionType().type() == TypeType::Box) {
        fg->setVariable(varId_, optional);
        auto vf = fg->builder().CreateExtractValue(optional, 0);
        return fg->builder().CreateICmpNE(vf, llvm::Constant::getNullValue(vf->getType()));
    }

    auto value = fg->buildGetOptionalValue(optional, expr_->expressionType());
    fg->setVariable(varId_, value);
    return fg->buildOptionalHasValue(optional, expr_->expressionType());
}

Value* ASTSuper::generate(FunctionCodeGenerator *fg) const {
    auto castedThis = fg->builder().CreateBitCast(fg->thisValue(), fg->typeHelper().llvmTypeFor(calleeType_));
    auto ret = CallCodeGenerator(fg, CallType::StaticDispatch).generate(castedThis, calleeType_, args_, function_,
                                                                        manageErrorProneness_ ? fg->errorPointer() :
                                                                        errorPointer());
    if (manageErrorProneness_) {
        fg->createIfElseBranchCond(isError(fg, fg->errorPointer()), [&]() {  // TODO: finish
            buildDestruct(fg);
            fg->builder().CreateRet(llvm::UndefValue::get(fg->llvmReturnType()));
            return false;
        }, [] { return true; });
    }

    return init_ ? nullptr : ret;
}

Value* ASTCallableCall::generate(FunctionCodeGenerator *fg) const {
    auto callable = callable_->generate(fg);
    auto type = callable_->expressionType();

    auto returnType = fg->typeHelper().llvmTypeFor(type.returnType());
    std::vector<llvm::Type *> argTypes { llvm::Type::getInt8PtrTy(fg->generator()->context()) };
    std::transform(type.parameters(), type.parametersEnd(), std::back_inserter(argTypes), [fg](auto &arg) {
        return fg->typeHelper().llvmTypeFor(arg);
    });
    auto functionType = llvm::FunctionType::get(returnType, argTypes, false);

    auto function = fg->builder().CreateBitCast(fg->builder().CreateExtractValue(callable, 0),
                                                functionType->getPointerTo());
    std::vector<llvm::Value *> args{ fg->builder().CreateExtractValue(callable, 1) };
    for (auto &arg : args_.args()) {
        args.emplace_back(arg->generate(fg));
    }
    return handleResult(fg, fg->builder().CreateCall(functionType, function, args));
}

}  // namespace EmojicodeCompiler
