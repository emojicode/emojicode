//
//  ASTTypeExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "../Generation/FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTTypeFromExpr::generateExpr(FnCodeGenerator *fncg) const {
    return expr_->generate(fncg);
}

Value* ASTStaticType::generateExpr(FnCodeGenerator *fncg) const {
    assert(availability() == TypeAvailability::StaticAndUnavailable ||
           availability() == TypeAvailability::StaticAndAvailabale);
    return nullptr;
}
    
Value* ASTThisType::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->thisValue();
}

}  // namespace EmojicodeCompiler
