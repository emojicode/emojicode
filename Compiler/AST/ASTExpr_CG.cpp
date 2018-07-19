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
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

Value* ASTTypeAsValue::generate(FunctionCodeGenerator *fg) const {
    if (type_->type().type() == TypeType::Class) {
        return type_->type().klass()->classInfo();
    }
    return llvm::UndefValue::get(fg->typeHelper().llvmTypeFor(Type(MakeTypeAsValue, type_->type())));
}

Value* ASTSizeOf::generate(FunctionCodeGenerator *fg) const {
    return fg->sizeOf(fg->typeHelper().llvmTypeFor(type_->type()));
}

Value* ASTCast::generate(FunctionCodeGenerator *fg) const {
    if (castType_ == CastType::ClassDowncast) {
        return downcast(fg);
    }

    auto box = fg->createEntryAlloca(fg->typeHelper().box());
    fg->builder().CreateStore(value_->generate(fg), box);
    Value *is = nullptr;
    switch (castType_) {
        case CastType::ToClass:
            is = castToClass(fg, box);
            break;
        case CastType::ToValueType:
            is = castToValueType(fg, box);
            break;
        case CastType::ToProtocol:
            // TODO: implement
            throw std::logic_error("Unimplemented");
        case CastType::ClassDowncast:
            throw std::logic_error("unreachable");
    }

    fg->createIfElse(is, []() {}, [fg, box]() {
        fg->getMakeNoValue(box);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTCast::downcast(FunctionCodeGenerator *fg) const {
    auto value = value_->generate(fg);
    auto info = fg->getClassInfoFromObject(value);
    auto toType = typeExpr_->expressionType();
    auto inheritsFrom = fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                                 { info, typeExpr_->generate(fg) });
    return fg->createIfElsePhi(inheritsFrom, [toType, fg, value]() {
        auto casted = fg->builder().CreateBitCast(value, fg->typeHelper().llvmTypeFor(toType));
        return fg->getSimpleOptionalWithValue(casted, toType.optionalized());
    }, [fg, toType]() {
        return fg->getSimpleOptionalWithoutValue(toType.optionalized());
    });
}

Value* ASTCast::castToValueType(FunctionCodeGenerator *fg, Value *box) const {
    return fg->builder().CreateICmpEQ(fg->builder().CreateLoad(fg->getBoxInfoPtr(box)), typeExpr_->generate(fg));
}

Value* ASTCast::castToClass(FunctionCodeGenerator *fg, Value *box) const {
    auto info = fg->getBoxInfoPtr(box);
    auto toType = typeExpr_->expressionType();
    auto expBoxInfo = fg->generator()->boxInfoFor(toType);

    return fg->createIfElsePhi(fg->builder().CreateICmpEQ(fg->builder().CreateLoad(info), expBoxInfo), [&]() {
        auto boxValue = fg->builder().CreateLoad(fg->getValuePtr(box, typeExpr_->expressionType()));
        auto obj = fg->builder().CreateBitCast(boxValue, fg->typeHelper().llvmTypeFor(toType));
        return fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                        { fg->getClassInfoFromObject(obj), typeExpr_->generate(fg) });
    }, [fg] {
        return llvm::ConstantInt::getFalse(fg->generator()->context());
    });
}

Value* ASTConditionalAssignment::generate(FunctionCodeGenerator *fg) const {
    auto optional = expr_->generate(fg);

    auto value = fg->builder().CreateExtractValue(optional, 1, "condValue");
    fg->scoper().getVariable(varId_) = LocalVariable(false, value);

    auto flag = fg->builder().CreateExtractValue(optional, 0);
    auto constant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(fg->generator()->context()), 1);
    return fg->builder().CreateICmpEQ(flag, constant);
}

Value* ASTSuper::generate(FunctionCodeGenerator *fg) const {
    auto castedThis = fg->builder().CreateBitCast(fg->thisValue(), fg->typeHelper().llvmTypeFor(calleeType_));
    auto ret = CallCodeGenerator(fg, CallType::StaticDispatch).generate(castedThis, calleeType_, args_, function_);

    if (manageErrorProneness_) {
        fg->createIfElseBranchCond(fg->getIsError(ret), [fg, ret]() {
            auto enumValue = fg->builder().CreateExtractValue(ret, 0);
            fg->builder().CreateRet(fg->getSimpleErrorWithError(enumValue, fg->llvmReturnType()));
            return false;
        }, []() { return true; });
    }

    return init_ ? nullptr : ret;
}

Value* ASTCallableCall::generate(FunctionCodeGenerator *fg) const {
    auto callable = callable_->generate(fg);

    auto genericArgs = callable_->expressionType().genericArguments();
    auto returnType = fg->typeHelper().llvmTypeFor(genericArgs.front());
    std::vector<llvm::Type *> argTypes { llvm::Type::getInt8PtrTy(fg->generator()->context()) };
    std::transform(genericArgs.begin() + 1, genericArgs.end(), std::back_inserter(argTypes), [fg](auto &arg) {
        return fg->typeHelper().llvmTypeFor(arg);
    });
    auto functionType = llvm::FunctionType::get(returnType, argTypes, false);

    auto function = fg->builder().CreateBitCast(fg->builder().CreateExtractValue(callable, 0),
                                                functionType->getPointerTo());
    std::vector<llvm::Value *> args{ fg->builder().CreateExtractValue(callable, 1) };
    for (auto &arg : args_.parameters()) {
        args.emplace_back(arg->generate(fg));
    }
    return fg->builder().CreateCall(functionType, function, args);
}

}  // namespace EmojicodeCompiler
