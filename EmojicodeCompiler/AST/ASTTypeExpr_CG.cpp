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
    if (type_.type() == TypeType::Class) {
        // TODO: type_.eclass()->index
    }
    else {
        assert(availability() == TypeAvailability::StaticAndUnavailable);
    }
    return nullptr;
}
    
Value* ASTThisType::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->thisValue();
}

}  // namespace EmojicodeCompiler
