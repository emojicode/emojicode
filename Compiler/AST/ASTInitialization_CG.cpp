//
//  _CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Generation/FunctionCodeGenerator.hpp"
#include "ASTInitialization.hpp"
#include "ASTTypeExpr.hpp"
#include "Functions/CallType.h"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/TypeDescriptionGenerator.hpp"
#include "Generation/Declarator.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"

namespace EmojicodeCompiler {

Value* ASTInitialization::generate(FunctionCodeGenerator *fg) const {
    switch (initType_) {
        case InitType::Class:
        case InitType::ClassStack:
            return generateClassInit(fg);
        case InitType::Enum:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fg->ctx()),
                                          typeExpr_->expressionType().enumeration()->getValueFor(name_).second);
        case InitType::ValueType:
            return generateInitValueType(fg);
        case InitType::MemoryAllocation:
            return generateMemoryAllocation(fg);
    }
}

Value* ASTInitialization::genericArgs(FunctionCodeGenerator *fg) const {
    return TypeDescriptionGenerator(fg).generate(typeExpr_->expressionType().selfResolvedGenericArgs());
}

Value* ASTInitialization::generateClassInit(FunctionCodeGenerator *fg) const {
    llvm::Value *obj;
    if (typeExpr_->expressionType().isExact()) {
        auto storesGenericArgs = fg->typeHelper().storesGenericArgs(typeExpr_->expressionType());
        if (typeExpr_->expressionType().klass()->foreign()) {
            assert(!storesGenericArgs);
            obj = CallCodeGenerator(fg, CallType::StaticContextfreeDispatch)
                .generate(nullptr, typeExpr_->expressionType(), args_, initializer_, errorPointer());
        }
        else {
            auto gargs = storesGenericArgs ? genericArgs(fg) : nullptr;
            obj = initObject(fg, args_, initializer_, typeExpr_->expressionType(), errorPointer(),
                             initType_ == InitType::ClassStack, gargs);
        }
    }
    else {
        obj = CallCodeGenerator(fg, CallType::DynamicDispatchOnType)
            .generate(typeExpr_->generate(fg), typeExpr_->expressionType(), args_, initializer_, errorPointer());
    }
    handleResult(fg, obj);
    return obj;
}

Value* ASTInitialization::generateInitValueType(FunctionCodeGenerator *fg) const {
    auto destination = vtDestination_;
    if (vtDestination_ == nullptr) {
        destination = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(typeExpr_->expressionType()));
    }

    auto storesGenericArgs = fg->typeHelper().storesGenericArgs(typeExpr_->expressionType());
    auto suppl = storesGenericArgs ? std::vector<llvm::Value*> { genericArgs(fg) } : std::vector<llvm::Value*>();
    CallCodeGenerator(fg, CallType::StaticDispatch)
            .generate(destination, typeExpr_->expressionType(), args_, initializer_, errorPointer(), suppl);
    handleResult(fg, nullptr, destination);
    return vtDestination_ == nullptr ? fg->builder().CreateLoad(destination) : nullptr;
}

Value* ASTInitialization::initObject(FunctionCodeGenerator *fg, const ASTArguments &args, Function *function,
                                     const Type &type, llvm::Value *errorPointer, bool stackInit,
                                     llvm::Value *gArgsDescs) {
    auto llvmType = llvm::dyn_cast<llvm::PointerType>(fg->typeHelper().llvmTypeFor(type));
    auto obj = stackInit ? fg->stackAlloc(llvmType) : fg->alloc(llvmType);
    fg->builder().CreateStore(type.klass()->classInfo(), fg->buildGetClassInfoPtrFromObject(obj));
    auto suppl = gArgsDescs != nullptr ? std::vector<llvm::Value*> { gArgsDescs } : std::vector<llvm::Value*>();
    return CallCodeGenerator(fg, CallType::StaticDispatch).generate(obj, type, args, function, errorPointer, suppl);
}

Value* ASTInitialization::generateMemoryAllocation(FunctionCodeGenerator *fg) const {
    auto size = fg->builder().CreateAdd(args_.args()[0]->generate(fg),
                                        fg->sizeOf(llvm::Type::getInt8PtrTy(fg->ctx())));
    return fg->builder().CreateCall(fg->generator()->declarator().alloc(), size, "alloc");
}

}  // namespace EmojicodeCompiler
