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

llvm::Value *CallCodeGenerator::generate(llvm::Value *callee, const Type &type, const ASTArguments &astArgs,
                                         Function *function) {
    auto args = createArgsVector(callee, astArgs);

    assert(function != nullptr);
    switch (callType_) {
        case CallType::StaticContextfreeDispatch:
        case CallType::StaticDispatch: {
            auto llvmFn = function->reificationFor(astArgs.genericArgumentTypes()).function;
            llvm::Type *castTo = nullptr;
            if (!args.empty() && args.front()->getType() != llvmFn->args().begin()->getType()) {
                if (function->functionType() == FunctionType::ObjectInitializer) {
                    castTo = args.front()->getType();
                }
                args.front() = fg()->builder().CreateBitCast(args.front(), llvmFn->args().begin()->getType());
            }
            auto ret = fg_->builder().CreateCall(llvmFn, args);
            return castTo == nullptr ? ret : fg_->builder().CreateBitCast(ret, castTo);
        }
        case CallType::DynamicDispatch:
        case CallType::DynamicDispatchOnType:
            assert(type.type() == TypeType::Class);
            return createDynamicDispatch(function, args, astArgs.genericArgumentTypes());
        case CallType::DynamicProtocolDispatch: {
            assert(type.type() == TypeType::Box);

            llvm::Value *conformance;
            if (type.boxedFor().type() != TypeType::Protocol) {
                conformance = buildFindProtocolConformance(args, type.unboxed());
            }
            else {
                auto conformanceType = fg()->typeHelper().protocolConformance();
                auto ptr = fg()->builder().CreateBitCast(fg()->buildGetBoxInfoPtr(args.front()),
                                                         conformanceType->getPointerTo()->getPointerTo());
                conformance = fg()->builder().CreateLoad(ptr);
            }
            return createDynamicProtocolDispatch(function, args, astArgs.genericArgumentTypes(), conformance);
        }
        case CallType::None:
            throw std::domain_error("CallType::None is not a valid call type");
    }
}

llvm::Value* CallCodeGenerator::buildFindProtocolConformance(const std::vector<llvm::Value *> &args,
                                                             const Type &protocol) {
    auto boxInfo = fg()->builder().CreateLoad(fg()->buildGetBoxInfoPtr(args.front()));
    auto id = fg()->generator()->protocolIdentifierFor(protocol);
    return fg()->buildFindProtocolConformance(args.front(), boxInfo, id);
}

std::vector<Value *> CallCodeGenerator::createArgsVector(llvm::Value *callee, const ASTArguments &args) const {
    std::vector<Value *> argsVector;
    if (callType_ != CallType::StaticContextfreeDispatch) {
        argsVector.emplace_back(callee);
    }
    for (auto &arg : args.args()) {
        argsVector.emplace_back(arg->generate(fg_));
    }
    return argsVector;
}

llvm::Value *MultiprotocolCallCodeGenerator::generate(llvm::Value *callee, const Type &calleeType,
                                                      const ASTArguments &args, Function* function,
                                                      size_t multiprotocolN) {
    assert(calleeType.type() == TypeType::Box);
    assert(function != nullptr);

    auto argsv = createArgsVector(callee, args);

    llvm::Value *conformance;
    if (calleeType.boxedFor().type() != TypeType::MultiProtocol) {
        conformance = buildFindProtocolConformance(argsv, calleeType.protocols()[multiprotocolN]);
    }
    else {
        auto mpt = fg()->typeHelper().multiprotocolConformance(calleeType);
        auto mp = fg()->builder().CreateBitCast(fg()->buildGetBoxInfoPtr(argsv.front()),
                                                mpt->getPointerTo()->getPointerTo());
        auto mpl = fg()->builder().CreateLoad(mp);

        conformance = fg()->builder().CreateLoad(fg()->builder().CreateConstGEP2_32(mpt, mpl, 0, multiprotocolN));
    }
    return createDynamicProtocolDispatch(function, std::move(argsv), args.genericArgumentTypes(), conformance);
}

llvm::Value *CallCodeGenerator::dispatchFromVirtualTable(Function *function, llvm::Value *virtualTable,
                                                         const std::vector<llvm::Value *> &args,
                                                         const std::vector<Type> &genericArguments) {
    auto reification = function->reificationFor(genericArguments);
    auto id = fg()->int32(reification.vti());
    auto dispatchedFunc = fg()->builder().CreateLoad(fg()->builder().CreateInBoundsGEP(virtualTable, id));

    std::vector<llvm::Type *> argTypes = reification.functionType()->params();
    if (callType_ == CallType::DynamicDispatch) {
        argTypes.front() = args.front()->getType();
    }

    auto funcType = llvm::FunctionType::get(reification.functionType()->getReturnType(), argTypes, false);
    auto func = fg()->builder().CreateBitCast(dispatchedFunc, funcType->getPointerTo(), "dispatchFunc");
    return fg_->builder().CreateCall(funcType, func, args);
}

llvm::Value *CallCodeGenerator::createDynamicDispatch(Function *function, const std::vector<llvm::Value *> &args,
                                                      const std::vector<Type> &genericArgs) {
    auto info = callType_ == CallType::DynamicDispatchOnType ? args.front() : fg()->buildGetClassInfoFromObject(args.front());
    auto tablePtr = fg()->builder().CreateConstInBoundsGEP2_32(fg_->typeHelper().classInfo(), info, 0, 1);
    auto table = fg()->builder().CreateLoad(tablePtr, "table");
    return dispatchFromVirtualTable(function, table, args, genericArgs);
}

llvm::Value *CallCodeGenerator::createDynamicProtocolDispatch(Function *function, std::vector<llvm::Value *> args,
                                                              const std::vector<Type> &genericArgs,
                                                              llvm::Value *conformance) {
    args.front() = getProtocolCallee(args, conformance);

    auto table = fg()->builder().CreateLoad(fg()->builder().CreateConstGEP2_32(conformance->getType()->getPointerElementType(),
                                                                               conformance, 0, 1), "table");
    return dispatchFromVirtualTable(function, table, args, genericArgs);
}

llvm::Value *CallCodeGenerator::getProtocolCallee(std::vector<Value *> &args, llvm::Value *conformance) const {
    auto shouldLoadPtr = fg()->builder().CreateConstGEP2_32(fg()->typeHelper().protocolConformance(),
                                                            conformance, 0, 0);
    return fg()->createIfElsePhi(fg()->builder().CreateLoad(shouldLoadPtr, "shouldLoad"), [this, &args]() {
        auto type = llvm::Type::getInt8PtrTy(fg()->generator()->context())->getPointerTo();
        auto value = fg()->buildGetBoxValuePtr(args.front(), type);
        return fg()->builder().CreateLoad(value);
    }, [this, &args]() {
        return fg()->buildGetBoxValuePtr(args.front(), llvm::Type::getInt8PtrTy(fg()->generator()->context()));
    });
}

}  // namespace EmojicodeCompiler
