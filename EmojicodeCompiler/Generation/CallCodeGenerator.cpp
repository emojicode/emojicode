//
//  CallCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CallCodeGenerator.hpp"
#include "../AST/ASTExpr.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/TypeDefinition.hpp"
#include "FunctionCodeGenerator.hpp"
#include <stdexcept>

namespace EmojicodeCompiler {

llvm::Value* CallCodeGenerator::generate(Value *callee, const Type &calleeType, const ASTArguments &args,
                                         const std::u32string &name) {
    std::vector<Value *> argsVector;
    if (callType_ != CallType::StaticContextfreeDispatch) {
        argsVector.emplace_back(callee);
    }
    for (auto &arg : args.arguments()) {
        argsVector.emplace_back(arg->generate(fncg_));
    }
    return createCall(argsVector, calleeType, name);
}

Function* CallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name){
    return type.typeDefinition()->lookupMethod(name);
}

llvm::Value* CallCodeGenerator::createCall(const std::vector<Value *> &args, const Type &type, const std::u32string &name) {
    auto function = lookupFunction(type, name);
    switch (callType_) {
        case CallType::StaticContextfreeDispatch:
        case CallType::StaticDispatch:
            return fncg_->builder().CreateCall(lookupFunction(type, name)->llvmFunction(), args);
        case CallType::DynamicDispatch:
            assert(type.type() == TypeType::Class);
            return createDynamicDispatch(function, args);
        case CallType::DynamicProtocolDispatch:
            // TODO: implement
            return nullptr;
        case CallType::None:
            throw std::domain_error("CallType::None is not a valid call type");
    }
}

llvm::Value* CallCodeGenerator::createDynamicDispatch(Function *function, const std::vector<Value *> &args) {
    auto meta = fncg()->getMetaFromObject(args.front());

    std::vector<Value *> idx2{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg()->generator()->context()), 0),  // classMeta
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg()->generator()->context()), 1),  // table
    };
    auto table = fncg()->builder().CreateLoad(fncg()->builder().CreateGEP(meta, idx2), "table");

    std::vector<Value *> idx3{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg()->generator()->context()), function->getVti())
    };
    auto dispatchedFunc = fncg()->builder().CreateLoad(fncg()->builder().CreateGEP(table, idx3));

    auto funcPointerType = function->llvmFunction()->getFunctionType()->getPointerTo();
    auto func = fncg()->builder().CreateBitCast(dispatchedFunc, funcPointerType, "dispatchFunc");
    return fncg_->builder().CreateCall(function->llvmFunction()->getFunctionType(), func, args);
}

Function* TypeMethodCallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name) {
    return type.typeDefinition()->lookupTypeMethod(name);
}

Function* InitializationCallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name) {
    return type.typeDefinition()->lookupInitializer(name);
}

}  // namespace EmojicodeCompiler
