//
//  ASTMethod_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTMethod::generate(FunctionCodeGenerator *fg) const {
    if (builtIn_ != BuiltInType::None) {
        auto v = callee_->generate(fg);
        switch (builtIn_) {
            case BuiltInType::IntegerNot:
                return fg->builder().CreateNot(v);
            case BuiltInType::IntegerToDouble:
                return fg->builder().CreateSIToFP(v, llvm::Type::getDoubleTy(fg->generator()->context()));
            case BuiltInType::BooleanNegate:
                return fg->builder().CreateICmpEQ(llvm::ConstantInt::getFalse(fg->generator()->context()), v);
            case BuiltInType::Store: {
                auto type = args_.genericArguments().front()->type();
                auto ptr = buildMemoryAddress(fg, v, args_.args()[1]->generate(fg), type);
                auto val = args_.args().front()->generate(fg);
                fg->builder().CreateStore(val, ptr);
                if (type.isManaged()) {
                    fg->retain(fg->isManagedByReference(type) ? ptr : val, type);
                }
                return nullptr;
            }
            case BuiltInType::Load: {
                auto type = args_.genericArguments().front()->type();
                auto ptr = buildMemoryAddress(fg, v, args_.args().front()->generate(fg), type);
                auto val = fg->builder().CreateLoad(ptr);
                if (type.isManaged()) {
                    fg->retain(fg->isManagedByReference(type) ? ptr : val, type);
                }
                return val;
            }
            case BuiltInType::Release: {
                auto type = args_.genericArguments().front()->type();
                if (type.isManaged()) {
                    auto ptr = buildMemoryAddress(fg, v, args_.args().front()->generate(fg), type);
                    fg->release(fg->isManagedByReference(type) ? ptr : fg->builder().CreateLoad(ptr), type);
                }
                return nullptr;
            }
            case BuiltInType::MemoryMove: {
                fg->builder().CreateMemMove(buildAddOffsetAddress(fg, v, args_.args()[0]->generate(fg)),
                                            buildAddOffsetAddress(fg, args_.args()[1]->generate(fg),
                                                                  args_.args()[2]->generate(fg)),
                                            args_.args()[3]->generate(fg), 0);
                return nullptr;
            }
            case BuiltInType::MemorySet: {
                fg->builder().CreateMemSet(buildAddOffsetAddress(fg, v, args_.args()[1]->generate(fg)),
                                           args_.args()[0]->generate(fg), args_.args()[2]->generate(fg), 0);
                return nullptr;
            }
            case BuiltInType::Multiprotocol:
                return MultiprotocolCallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_, args_,
                                                                              method_, multiprotocolN_);
            default:
                break;
        }
    }

    return handleResult(fg, CallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_,
                                                                      args_, method_));
}

Value* ASTMethod::buildAddOffsetAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset) const {
    auto addOffset = fg->builder().CreateAdd(offset, fg->sizeOf(llvm::Type::getInt8PtrTy(fg->generator()->context())));
    return fg->builder().CreateGEP(memory, addOffset);
}

Value* ASTMethod::buildMemoryAddress(FunctionCodeGenerator *fg, llvm::Value *memory, llvm::Value *offset,
                                     const Type &type) const {
    auto ptrType = fg->typeHelper().llvmTypeFor(type)->getPointerTo();
    return fg->builder().CreateBitCast(buildAddOffsetAddress(fg, memory, offset), ptrType);
}

}  // namespace EmojicodeCompiler
