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
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fg->generator()->context()),
                                          typeExpr_->type().enumeration()->getValueFor(name_).second);
        case InitType::ValueType:
            return generateInitValueType(fg);
        case InitType::MemoryAllocation:
            return generateMemoryAllocation(fg);
    }
}

Value* ASTInitialization::generateClassInit(FunctionCodeGenerator *fg) const {
    llvm::Value *obj;
    if (typeExpr_->type().isExact()) {
        if (typeExpr_->type().klass()->foreign()) {
            obj = CallCodeGenerator(fg, CallType::StaticContextfreeDispatch)
                .generate(nullptr, typeExpr_->type(), args_, initializer_);
        }
        else {
            obj = initObject(fg, args_, initializer_, typeExpr_->type(), initType_ == InitType::ClassStack);
        }
    }
    else {
        obj = CallCodeGenerator(fg, CallType::DynamicDispatchOnType)
            .generate(typeExpr_->generate(fg), typeExpr_->type(), args_, initializer_);
    }
    handleResult(fg, obj);
    return obj;
}

Value* ASTInitialization::generateInitValueType(FunctionCodeGenerator *fg) const {
    auto destination = vtDestination_;
    if (vtDestination_ == nullptr) {
        destination = fg->createEntryAlloca(fg->typeHelper().llvmTypeFor(typeExpr_->type()));
    }
    CallCodeGenerator(fg, CallType::StaticDispatch)
            .generate(destination, typeExpr_->type(), args_, initializer_);
    handleResult(fg, nullptr, destination);
    return vtDestination_ == nullptr ? fg->builder().CreateLoad(destination) : nullptr;
}

Value* ASTInitialization::initObject(FunctionCodeGenerator *fg, const ASTArguments &args, Function *function,
                                     const Type &type, bool stackInit) {
    auto llvmType = llvm::dyn_cast<llvm::PointerType>(fg->typeHelper().llvmTypeFor(type));
    auto obj = stackInit ? fg->stackAlloc(llvmType) : fg->alloc(llvmType);
    fg->builder().CreateStore(type.klass()->reificationFor(type.genericArguments()).classInfo,
                              fg->buildGetClassInfoPtrFromObject(obj));
    return CallCodeGenerator(fg, CallType::StaticDispatch).generate(obj, type, args, function);
}

Value* ASTInitialization::generateMemoryAllocation(FunctionCodeGenerator *fg) const {
    auto size = fg->builder().CreateAdd(args_.args()[0]->generate(fg),
                                        fg->sizeOf(llvm::Type::getInt8PtrTy(fg->generator()->context())));
    return fg->builder().CreateCall(fg->generator()->declarator().alloc(), size, "alloc");
}

}  // namespace EmojicodeCompiler
