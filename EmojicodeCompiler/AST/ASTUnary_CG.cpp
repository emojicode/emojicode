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
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), 0);
    return fncg->builder().CreateICmpEQ(vf, fncg->generator()->optionalNoValue());
}

Value* ASTIsError::generateExpr(FnCodeGenerator *fncg) const {
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), 0);
    return fncg->builder().CreateICmpEQ(vf, fncg->generator()->optionalNoValue());
}

Value* ASTUnwrap::generateExpr(FnCodeGenerator *fncg) const {
    auto vf = fncg->builder().CreateExtractValue(value_->generate(fncg), 1);

    // TODO: box
    return vf;
}

Value* ASTMetaTypeFromInstance::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->getMetaFromObject(value_->generate(fncg));
}

}  // namespace EmojicodeCompiler
