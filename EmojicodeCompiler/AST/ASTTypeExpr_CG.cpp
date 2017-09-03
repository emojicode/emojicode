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

void ASTTypeFromExpr::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
}


void ASTStaticType::generateExpr(FnCodeGenerator *fncg) const {
    if (type_.type() == TypeType::Class) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    }
    else {
        assert(availability() == TypeAvailability::StaticAndUnavailable);
    }
}

void ASTThisType::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_THIS);
}

}  // namespace EmojicodeCompiler
