//
//  ASTUnary_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"

namespace EmojicodeCompiler {

void ASTIsNothigness::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_IS_NOTHINGNESS);
}

void ASTIsError::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_IS_ERROR);
}

void ASTUnwrap::generateExpr(FnCodeGenerator *fncg) const {
    value_->generate(fncg);
    auto type = value_->expressionType();
    if (type.storageType() == StorageType::Box) {
        fncg->wr().writeInstruction(error_ ? INS_ERROR_CHECK_BOX_OPTIONAL : INS_UNWRAP_BOX_OPTIONAL);
    }
    else {
        fncg->wr().writeInstruction(error_ ? INS_ERROR_CHECK_SIMPLE_OPTIONAL : INS_UNWRAP_SIMPLE_OPTIONAL);
        fncg->wr().writeInstruction(type.size() - 1);
    }
}

void ASTMetaTypeFromInstance::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_GET_CLASS_FROM_INSTANCE);
}

}  // namespace EmojicodeCompiler
