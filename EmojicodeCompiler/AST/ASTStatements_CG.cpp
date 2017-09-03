//
//  ASTStatements_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Scoping/CGScoper.hpp"
#include "../../EmojicodeInstructions.h"

namespace EmojicodeCompiler {

void ASTBlock::generate(FnCodeGenerator *fncg) const {
    for (auto &stmt : stmts_) {
        stmt->generate(fncg);
    }
}

void ASTReturn::generate(FnCodeGenerator *fncg) const {
    if (value_) {
        value_->generate(fncg);
    }
    fncg->wr().writeInstruction(INS_RETURN);
}

void ASTRaise::generate(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_PUSH_ERROR);
    value_->generate(fncg);
    if (boxed_) {
        fncg->wr().writeInstruction(INS_PUSH_N),
        fncg->wr().writeInstruction(kBoxValueSize - value_->expressionType().size() - 1);
    }
    fncg->wr().writeInstruction(INS_RETURN);
}

void ASTSuperinitializer::generate(FnCodeGenerator *fncg) const {
    SuperInitializerCallCodeGenerator(fncg, INS_SUPER_INITIALIZER).generate(superType_, arguments_, name_);
    fncg->wr().writeInstruction(INS_POP);
}

}  // namespace EmojicodeCompiler
