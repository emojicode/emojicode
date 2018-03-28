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
                auto pointerType = fg->typeHelper().llvmTypeFor(args_.genericArguments().front())->getPointerTo();
                auto offset = args_.parameters()[1]->generate(fg);
                auto ptr = fg->builder().CreateBitCast(fg->builder().CreateGEP(v, offset), pointerType);
                return fg->builder().CreateStore(args_.parameters().front()->generate(fg), ptr);
            }
            case BuiltInType::Load: {
                auto pointerType = fg->typeHelper().llvmTypeFor(args_.genericArguments().front())->getPointerTo();
                auto offset = args_.parameters().front()->generate(fg);
                auto ptr = fg->builder().CreateBitCast(fg->builder().CreateGEP(v, offset), pointerType);
                return fg->builder().CreateLoad(ptr);
            }
            case BuiltInType::TypeMethod:
                return TypeMethodCallCodeGenerator(fg, callType_).generate(callee_->generate(fg),
                                                                           calleeType_, args_, name_);
            default:
                break;
        }
    }

    return CallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_, args_, name_);
}
    
}  // namespace EmojicodeCompiler
