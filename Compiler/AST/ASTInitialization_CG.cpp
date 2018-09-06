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
#include "Types/Class.hpp"
#include "Types/Enum.hpp"

namespace EmojicodeCompiler {

Value* ASTInitialization::generate(FunctionCodeGenerator *fg) const {
    switch (initType_) {
        case InitType::Class:
        case InitType::ClassStack:
            return generateClassInit(fg);
        case InitType::Enum:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fg->generator()->context()),
                                          typeExpr_->expressionType().enumeration()->getValueFor(name_).second);
        case InitType::ValueType:
            return generateInitValueType(fg);
        case InitType::MemoryAllocation:
            return generateMemoryAllocation(fg);
    }
}

Value* ASTInitialization::generateClassInit(FunctionCodeGenerator *fg) const {
    llvm::Value *obj;
    if (typeExpr_->expressionType().klass()->foreign()) {
        obj = CallCodeGenerator(fg, CallType::StaticContextfreeDispatch)
                                .generate(nullptr, typeExpr_->expressionType(), args_, initializer_);
    }
    else if (typeExpr_->expressionType().isExact()) {
        obj = initObject(fg, args_, initializer_, typeExpr_->expressionType(), initType_ == InitType::ClassStack);
    }
    else {
        obj = CallCodeGenerator(fg, CallType::DynamicDispatchOnType)
            .generate(typeExpr_->generate(fg), typeExpr_->expressionType(), args_, initializer_);
    }
    handleResult(fg, obj, false, initType_ == InitType::ClassStack);
    return obj;
}

Value* ASTInitialization::generateInitValueType(FunctionCodeGenerator *fg) const {
    auto destination = vtDestination_;
    if (vtDestination_ == nullptr) {
        destination = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(typeExpr_->expressionType()));
    }
    CallCodeGenerator(fg, CallType::StaticDispatch)
            .generate(destination, typeExpr_->expressionType(), args_, initializer_);
    handleResult(fg, destination, true);
    return vtDestination_ == nullptr ? fg->builder().CreateLoad(destination) : nullptr;
}

Value* ASTInitialization::initObject(FunctionCodeGenerator *fg, const ASTArguments &args, Function *function,
                                     const Type &type, bool stackInit) {
    auto llvmType = llvm::dyn_cast<llvm::PointerType>(fg->typeHelper().llvmTypeFor(type));
    auto obj = stackInit ? fg->createEntryAlloca(llvmType->getElementType()) : fg->alloc(llvmType);
    if (stackInit) {
        auto controlBlockField = fg->builder().CreateConstInBoundsGEP2_32(llvmType->getElementType(), obj, 0, 0);
        fg->builder().CreateStore(llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(fg->generator()->context())),
                                  controlBlockField);
    }
    fg->builder().CreateStore(type.klass()->classInfo(), fg->buildGetClassInfoPtrFromObject(obj));
    return CallCodeGenerator(fg, CallType::StaticDispatch).generate(obj, type, args, function);
}

Value* ASTInitialization::generateMemoryAllocation(FunctionCodeGenerator *fg) const {
    auto size = fg->builder().CreateAdd(args_.parameters()[0]->generate(fg),
                                        fg->sizeOf(llvm::Type::getInt8PtrTy(fg->generator()->context())));
    return fg->builder().CreateCall(fg->generator()->declarator().alloc(), size, "alloc");
}

}  // namespace EmojicodeCompiler
