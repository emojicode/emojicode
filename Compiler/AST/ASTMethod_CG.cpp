//
//  ASTMethod_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "ASTType.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

llvm::Value* callIntrinsic(FunctionCodeGenerator *fg, llvm::Intrinsic::ID id, llvm::ArrayRef<llvm::Value *> args) {
    return fg->builder().CreateIntrinsic(id, args.front()->getType(), args);
}

Value* ASTMethod::generate(FunctionCodeGenerator *fg) const {
    if (builtIn_ != BuiltInType::None) {
        auto v = callee_->generate(fg);
        switch (builtIn_) {
            case BuiltInType::IntegerNot:
                return fg->builder().CreateNot(v);
            case BuiltInType::IntegerToDouble:
                return fg->builder().CreateSIToFP(v, llvm::Type::getDoubleTy(fg->ctx()));
            case BuiltInType::IntegerInverse:
                return fg->builder().CreateMul(v, llvm::ConstantInt::get(v->getType(), -1));
            case BuiltInType::IntegerToByte:
                return fg->builder().CreateTrunc(v, llvm::Type::getInt8Ty(fg->ctx()));
            case BuiltInType::ByteToInteger:
                return fg->builder().CreateSExt(v, llvm::Type::getInt64Ty(fg->ctx()));
            case BuiltInType::DoubleInverse:
                return fg->builder().CreateFNeg(v);
            case BuiltInType::Power:
                return callIntrinsic(fg, llvm::Intrinsic::ID::pow, {v, args_.args()[0]->generate(fg)});
            case BuiltInType::Log2:
                return callIntrinsic(fg, llvm::Intrinsic::ID::log2, v);
            case BuiltInType::Log10:
                return callIntrinsic(fg, llvm::Intrinsic::ID::log10, v);
            case BuiltInType::Ln:
                return callIntrinsic(fg, llvm::Intrinsic::ID::log, v);
            case BuiltInType::Ceil:
                return callIntrinsic(fg, llvm::Intrinsic::ID::ceil, v);
            case BuiltInType::Floor:
                return callIntrinsic(fg, llvm::Intrinsic::ID::floor, v);
            case BuiltInType::Round:
                return callIntrinsic(fg, llvm::Intrinsic::ID::round, v);
            case BuiltInType::DoubleAbs:
                return callIntrinsic(fg, llvm::Intrinsic::ID::fabs, v);
            case BuiltInType::DoubleToInteger:
                return fg->builder().CreateFPToSI(v, llvm::Type::getInt64Ty(fg->ctx()));
            case BuiltInType::BooleanNegate:
                return fg->builder().CreateICmpEQ(llvm::ConstantInt::getFalse(fg->ctx()), v);
            case BuiltInType::Store: {
                auto type = args_.genericArguments().front()->type();
                auto ptr = buildMemoryAddress(fg, v, args_.args()[1]->generate(fg), type);
                auto val = args_.args().front()->generate(fg);
                auto store = fg->builder().CreateStore(val, ptr);
                if (fg->typeHelper().shouldAddTbaa(type)) {
                    auto tbaa = fg->typeHelper().tbaaNodeFor(type, false);
                    auto accessTag = fg->typeHelper().mdBuilder()->createTBAAAccessTag(tbaa, tbaa, 0, 0);
                    store->setMetadata(llvm::LLVMContext::MD_tbaa, accessTag);
                }
                if (type.isManaged()) {
                    fg->retain(fg->isManagedByReference(type) ? ptr : val, type);
                }
                return nullptr;
            }
            case BuiltInType::Load: {
                auto type = args_.genericArguments().front()->type();
                return buildMemoryAddress(fg, v, args_.args().front()->generate(fg), type);
            }
            case BuiltInType::Release: {
                auto type = args_.genericArguments().front()->type();
                if (type.isManaged()) {
                    auto ptr = buildMemoryAddress(fg, v, args_.args().front()->generate(fg), type);
                    fg->releaseByReference(ptr, type);
                }
                return nullptr;
            }
            case BuiltInType::MemoryMove: {
                fg->builder().CreateMemMove(buildAddOffsetAddress(fg, v, args_.args()[0]->generate(fg)), 0,
                                            buildAddOffsetAddress(fg, args_.args()[1]->generate(fg),
                                                                  args_.args()[2]->generate(fg)), 0,
                                            args_.args()[3]->generate(fg));
                return nullptr;
            }
            case BuiltInType::MemorySet: {
                fg->builder().CreateMemSet(buildAddOffsetAddress(fg, v, args_.args()[1]->generate(fg)),
                                           args_.args()[0]->generate(fg), args_.args()[2]->generate(fg), 0);
                return nullptr;
            }
            case BuiltInType::Multiprotocol:
                return MultiprotocolCallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_, args_,
                                                                              method_, errorPointer(), multiprotocolN_);
            default:
                break;
        }
    }

    return handleResult(fg, CallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_,
                                                                      args_, method_, errorPointer()));
}

Value* ASTMethod::buildAddOffsetAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset) const {
    auto addOffset = fg->builder().CreateAdd(offset, fg->sizeOf(llvm::Type::getInt8PtrTy(fg->ctx())));
    return fg->builder().CreateGEP(memory, addOffset);
}

Value* ASTMethod::buildMemoryAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset,
                                     const Type &type) const {
    auto ptrType = fg->typeHelper().llvmTypeFor(type)->getPointerTo();
    return fg->builder().CreateBitCast(buildAddOffsetAddress(fg, memory, offset), ptrType);
}

}  // namespace EmojicodeCompiler
