//
//  CallCodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CallCodeGenerator.hpp"
#include "AST/ASTExpr.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeDefinition.hpp"
#include <llvm/Support/raw_ostream.h>
#include <stdexcept>

namespace EmojicodeCompiler {

llvm::Value* CallCodeGenerator::generate(Value *callee, const Type &calleeType, const ASTArguments &args,
                                         const std::u32string &name) {
    std::vector<Value *> argsVector;
    if (callType_ != CallType::StaticContextfreeDispatch) {
        argsVector.emplace_back(callee);
    }
    for (auto &arg : args.arguments()) {
        argsVector.emplace_back(arg->generate(fg_));
    }
    return createCall(argsVector, calleeType, name, args.isImperative());
}

Function* CallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name, bool imperative) {
    return type.typeDefinition()->lookupMethod(name, imperative);
}

llvm::Value * CallCodeGenerator::createCall(const std::vector<Value *> &args, const Type &type,
                                            const std::u32string &name, bool imperative) {
    auto function = lookupFunction(type, name, imperative);
    switch (callType_) {
        case CallType::StaticContextfreeDispatch:
        case CallType::StaticDispatch:
            return fg_->builder().CreateCall(function->llvmFunction(), args);
        case CallType::DynamicDispatch:
            assert(type.type() == TypeType::Class);
            return createDynamicDispatch(function, args);
        case CallType::DynamicProtocolDispatch:
            assert(type.type() == TypeType::Protocol);
            return createDynamicProtocolDispatch(function, args, type);
        case CallType::None:
            throw std::domain_error("CallType::None is not a valid call type");
    }
}

llvm::Value* CallCodeGenerator::dispatchFromVirtualTable(Function *function, llvm::Value *virtualTable,
                                                         const std::vector<llvm::Value *> &args) {
    std::vector<Value *> idx3{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(fg()->generator()->context()), function->vti())
    };
    auto dispatchedFunc = fg()->builder().CreateLoad(fg()->builder().CreateGEP(virtualTable, idx3));

    std::vector<llvm::Type *> argTypes = function->llvmFunctionType()->params();
    argTypes.front() = args.front()->getType();

    auto funcType = llvm::FunctionType::get(function->llvmFunctionType()->getReturnType(), argTypes, false);
    auto func = fg()->builder().CreateBitCast(dispatchedFunc, funcType->getPointerTo(), "dispatchFunc");
    return fg_->builder().CreateCall(funcType, func, args);
}

llvm::Value* CallCodeGenerator::createDynamicDispatch(Function *function, const std::vector<Value *> &args) {
    auto meta = fg()->getMetaFromObject(args.front());

    std::vector<Value *> idx2{ fg()->int32(0), fg()->int32(1) };  // classMeta.table
    auto table = fg()->builder().CreateLoad(fg()->builder().CreateGEP(meta, idx2), "table");
    return dispatchFromVirtualTable(function, table, args);
}

llvm::Value* CallCodeGenerator::createDynamicProtocolDispatch(Function *function, std::vector<Value *> args,
                                                              const Type &calleeType) {
    auto valueTypeMeta = fg()->builder().CreateLoad(fg()->getMetaTypePtr(args.front()));
    auto isClass = fg()->builder().CreateICmpEQ(valueTypeMeta, fg()->generator()->classValueTypeMeta());
    auto i8PtrType = llvm::Type::getInt8Ty(fg()->generator()->context())->getPointerTo();
    auto pair = fg()->createIfElsePhi(isClass, [this, args, calleeType, i8PtrType]() {
        auto gtype = fg()->typeHelper().classMeta()->getPointerTo()->getPointerTo()->getPointerTo();
        auto obj = fg()->builder().CreateLoad(fg()->getValuePtr(args.front(), gtype));
        auto meta = fg()->builder().CreateLoad(obj);

        std::vector<Value *> idx{ fg()->int32(0), fg()->int32(2) };  // classMeta.protocolsTable
        return std::make_pair(fg()->builder().CreateGEP(meta, idx, "protocolsTable"),
                              fg()->builder().CreateBitCast(obj, i8PtrType));
    }, [this, valueTypeMeta, args, i8PtrType]() {
        std::vector<Value *> idx{ fg()->int32(0), fg()->int32(0) };  // valueTypeMeta.protocolsTable
        return std::make_pair(fg()->builder().CreateGEP(valueTypeMeta, idx, "protocolsTable"),
                              fg()->getValuePtr(args.front(), i8PtrType));
    });

    auto protocolsTable = fg()->builder().CreateLoad(pair.first);
    auto protocolsVtables = fg()->builder().CreateExtractValue(protocolsTable, 0);

    auto pindex = fg()->int16(calleeType.protocol()->index());
    auto index = fg()->builder().CreateSub(pindex, fg()->builder().CreateExtractValue(protocolsTable, 1));
    auto vtable = fg()->builder().CreateLoad(fg()->builder().CreateGEP(protocolsVtables, index, "protocolVtable"));
    args.front() = pair.second;
    return dispatchFromVirtualTable(function, vtable, args);
}

Function* TypeMethodCallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name, bool imperative) {
    return type.typeDefinition()->lookupTypeMethod(name, imperative);
}

Function* InitializationCallCodeGenerator::lookupFunction(const Type &type, const std::u32string &name, bool imperative) {
    return type.typeDefinition()->lookupInitializer(name);
}

}  // namespace EmojicodeCompiler
