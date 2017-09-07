//
//  ASTMethod_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"

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
            default:
                break;
        }
    }

    return CallCodeGenerator(fg, callType_).generate(callee_->generate(fg), calleeType_,  args_, name_);
}
    
}  // namespace EmojicodeCompiler
