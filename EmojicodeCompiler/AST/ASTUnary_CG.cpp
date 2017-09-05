//
//  ASTUnary_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"

namespace EmojicodeCompiler {

Value* ASTIsNothigness::generateExpr(FnCodeGenerator *fncg) const {
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), std::vector<unsigned int>{0});
    auto constant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(fncg->generator()->context()), 0);
    return fncg->builder().CreateICmpEQ(vf, constant);
}

Value* ASTIsError::generateExpr(FnCodeGenerator *fncg) const {
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), std::vector<unsigned int>{0});
    auto constant = llvm::ConstantInt::get(llvm::Type::getInt1Ty(fncg->generator()->context()), 0);
    return fncg->builder().CreateICmpEQ(vf, constant);
}

Value* ASTUnwrap::generateExpr(FnCodeGenerator *fncg) const {
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), std::vector<unsigned int>{1});

    // TODO: box
    return vf;
}

Value* ASTMetaTypeFromInstance::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->getMetaFromObject(value_->generate(fncg));
}

}  // namespace EmojicodeCompiler
