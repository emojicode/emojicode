//
//  ASTBoxing.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBoxing.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Functions/BoxingLayer.hpp"

namespace EmojicodeCompiler {

void ASTBoxToSimpleOptional::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    auto rtype = expr_->expressionType();
    rtype.setOptional();
    rtype.unbox();
    if (rtype.remotelyStored()) {
        fncg->wr().writeInstruction({ INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE,
            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
    }
    else {
        fncg->wr().writeInstruction({ INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE,
            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
    }
}

void ASTSimpleToSimpleOptional::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_SIMPLE_OPTIONAL_PRODUCE);
    expr_->generate(fncg);
}

void ASTSimpleOptionalToBox::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    auto rtype = expr_->expressionType();
    if (rtype.remotelyStored()) {
        fncg->wr().writeInstruction({ INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE,
            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()),
            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
    }
    else {
        fncg->wr().writeInstruction({ INS_SIMPLE_OPTIONAL_TO_BOX,
            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()),
            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
    }
}

void ASTSimpleToBox::generateExpr(FnCodeGenerator *fncg) const {
    auto rtype = expr_->expressionType();
    if (rtype.remotelyStored()) {
        expr_->generate(fncg);
        fncg->wr().writeInstruction({ INS_BOX_PRODUCE_REMOTE,
            static_cast<EmojicodeInstruction>(rtype.size()),
            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()) });
    }
    else {
        fncg->wr().writeInstruction({ INS_BOX_PRODUCE,
            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()) });
        expr_->generate(fncg);
        fncg->wr().writeInstruction(INS_PUSH_N);
        fncg->wr().writeInstruction(kBoxValueSize - rtype.size() - 1);
    }
}

void ASTBoxToSimple::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    auto rtype = expr_->expressionType();
    rtype.unbox();
    if (rtype.remotelyStored()) {
        fncg->wr().writeInstruction({ INS_UNBOX_REMOTE, static_cast<EmojicodeInstruction>(rtype.size()) });
    }
    else {
        fncg->wr().writeInstruction({ INS_UNBOX, static_cast<EmojicodeInstruction>(rtype.size()) });
    }
}

void ASTDereference::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    fncg->wr().writeInstruction(INS_PUSH_VALUE_FROM_REFERENCE);
    fncg->wr().writeInstruction(expr_->expressionType().size());
}

void ASTCallableBox::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    fncg->wr().writeInstruction(INS_CLOSURE_BOX);
    fncg->wr().writeInstruction(boxingLayer_->vtiForUse());
}

void ASTStoreTemporarily::generateExpr(FnCodeGenerator *fncg) const {
    auto rtype = expr_->expressionType();
    auto &variable = fncg->scoper().declareVariable(varId_, rtype);
    expr_->generate(fncg);
    fncg->copyToVariable(variable.stackIndex, false, rtype);
    variable.initialize(fncg->wr().count());
    fncg->pushVariableReference(variable.stackIndex, false);
}
    
}  // namespace EmojicodeCompiler
