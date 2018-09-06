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

llvm::Value* ASTExpr::handleResult(FunctionCodeGenerator *fg, llvm::Value *result, bool valueTypeIsReferenced, bool local) const {
    if (isTemporary_ && expressionType().isManaged()) {
        if (!valueTypeIsReferenced && !expressionType().isReference() && fg->isManagedByReference(expressionType())) {
            auto temp = fg->createEntryAlloca(result->getType());
            fg->builder().CreateStore(result, temp);
            fg->addTemporaryObject(temp, expressionType(), false);
        }
        else {
            fg->addTemporaryObject(result, expressionType(), local);
        }
    }
    return result;
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
            return castToProtocol(fg, box);
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

Value* ASTCast::boxInfo(FunctionCodeGenerator *fg, Value *box) const {
    if (value_->expressionType().boxedFor().type() == TypeType::Protocol) {
        auto protocolConPtr = fg->builder().CreateBitCast(fg->builder().CreateLoad(fg->getBoxInfoPtr(box)),
                                                          fg->typeHelper().protocolConformance()->getPointerTo());
        return fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().protocolConformance(), protocolConPtr, 0, 2));
    }
    return fg->builder().CreateLoad(fg->getBoxInfoPtr(box));
}

Value* ASTCast::castToValueType(FunctionCodeGenerator *fg, Value *box) const {
    return fg->builder().CreateICmpEQ(boxInfo(fg, box), typeExpr_->generate(fg));
}

Value* ASTCast::castToClass(FunctionCodeGenerator *fg, Value *box) const {
    auto toType = typeExpr_->expressionType();
    auto isExpBoxInfo = fg->builder().CreateICmpEQ(boxInfo(fg, box), fg->boxInfoFor(toType));

    return fg->createIfElsePhi(isExpBoxInfo, [&] {
        auto obj = fg->builder().CreateLoad(fg->getValuePtr(box, typeExpr_->expressionType()));
        return fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                        { fg->getClassInfoFromObject(obj), typeExpr_->generate(fg) });
    }, [fg] {
        return llvm::ConstantInt::getFalse(fg->generator()->context());
    });
}

Value* ASTCast::castToProtocol(FunctionCodeGenerator *fg, Value *box) const {
    auto objBoxInfo = fg->builder().CreateBitCast(fg->generator()->declarator().boxInfoForObjects(),
                                                  fg->typeHelper().boxInfo()->getPointerTo());
    auto boxInfoValue = boxInfo(fg, box);
    auto conformanceEntries = fg->createIfElsePhi(fg->builder().CreateICmpEQ(boxInfoValue, objBoxInfo), [&]() {
        auto obj = fg->builder().CreateLoad(fg->getValuePtr(box, fg->typeHelper().someobject()->getPointerTo()));
        auto classInfo = fg->getClassInfoFromObject(obj);
        return fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().classInfo(),
                                                                                 classInfo, 0, 2));
    }, [&] {
        auto conformanceEntriesPtr = fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().boxInfo(), boxInfoValue, 0, 0);
        return fg->builder().CreateLoad(conformanceEntriesPtr);
    });

    auto conformance = fg->builder().CreateCall(fg->generator()->declarator().findProtocolConformance(),
                                                { conformanceEntries, typeExpr_->generate(fg) });
    auto confPtrTy = fg->typeHelper().protocolConformance()->getPointerTo();
    auto confNullptr = llvm::ConstantPointerNull::get(confPtrTy);

    fg->createIfElse(fg->builder().CreateICmpNE(conformance, confNullptr), [&] {
        auto infoPtr = fg->getBoxInfoPtr(box);
        fg->builder().CreateStore(conformance, fg->builder().CreateBitCast(infoPtr, confPtrTy->getPointerTo()));
    }, [&] {
        fg->getMakeNoValue(box);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTConditionalAssignment::generate(FunctionCodeGenerator *fg) const {
    auto optional = expr_->generate(fg);

    if (expr_->expressionType().type() == TypeType::Box) {
        fg->scoper().getVariable(varId_) = LocalVariable(false, optional);
        auto vf = fg->builder().CreateExtractValue(optional, 0);
        return fg->builder().CreateICmpNE(vf, llvm::Constant::getNullValue(vf->getType()));
    }

    auto value = fg->builder().CreateExtractValue(optional, 1, "condValue");
    fg->scoper().getVariable(varId_) = LocalVariable(false, value);
    return fg->buildOptionalHasValue(optional);
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
    return handleResult(fg, fg->builder().CreateCall(functionType, function, args));
}

}  // namespace EmojicodeCompiler
