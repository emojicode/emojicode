//
//  ASTVariables_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "../Generation/FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

void ASTInitableCreator::generate(FnCodeGenerator *fncg) const {
    if (noAction_) {
        expr_->generate(fncg);
    }
    else {
        generateAssignment(fncg);
    }
}

void ASTVariableDeclaration::generate(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(id_, type_);
    if (type_.optional()) {
        fncg->wr().writeInstruction(INS_GET_NOTHINGNESS);
        fncg->wr().writeInstruction(INS_COPY_TO_STACK);
        fncg->wr().writeInstruction(var.stackIndex.value());
    }
}

CGScoper::Variable& ASTVariableAssignmentDecl::generateGetVariable(FnCodeGenerator *fncg) const {
    if (declare_) {
        return fncg->scoper().declareVariable(varId_, expr_->expressionType());
    }
    return inInstanceScope() ? fncg->instanceScoper().getVariable(varId_) : fncg->scoper().getVariable(varId_);
}

void ASTVariableAssignmentDecl::generateAssignment(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);

    auto &var = generateGetVariable(fncg);
    fncg->copyToVariable(var.stackIndex, inInstanceScope(), expr_->expressionType());
    var.initialize(fncg->wr().count());
}

void ASTFrozenDeclaration::generateAssignment(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);

    auto &var = fncg->scoper().declareVariable(id_, expr_->expressionType());
    fncg->copyToVariable(var.stackIndex, false, expr_->expressionType());
    var.initialize(fncg->wr().count());
}

}  // namespace EmojicodeCompiler
