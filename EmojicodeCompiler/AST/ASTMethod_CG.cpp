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

Value* ASTMethod::generate(FunctionCodeGenerator *fncg) const {
    if (builtIn_ != BuiltInType::None) {
        auto v = callee_->generate(fncg);
        switch (builtIn_) {
            case BuiltInType::IntegerNot:
                return fncg->builder().CreateNot(v);
            case BuiltInType::IntegerToDouble:
                return fncg->builder().CreateSIToFP(v, llvm::Type::getDoubleTy(fncg->generator()->context()));
            case BuiltInType::BooleanNegate:
                return fncg->builder().CreateICmpEQ(llvm::ConstantInt::getFalse(fncg->generator()->context()), v);
            default:
                break;
        }
    }

    return CallCodeGenerator(fncg, callType_).generate(callee_->generate(fncg), calleeType_,  args_, name_);
}
    
}  // namespace EmojicodeCompiler
