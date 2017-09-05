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

namespace EmojicodeCompiler {

void ASTBlock::generate(FnCodeGenerator *fncg) const {
    for (auto &stmt : stmts_) {
        stmt->generate(fncg);
    }
}

void ASTReturn::generate(FnCodeGenerator *fncg) const {
    if (value_) {
        fncg->builder().CreateRet(value_->generate(fncg));
    }
    else {
        fncg->builder().CreateRetVoid();
    }
}

void ASTRaise::generate(FnCodeGenerator *fncg) const {
    // TODO: implement
}

void ASTSuperinitializer::generate(FnCodeGenerator *fncg) const {
    SuperInitializerCallCodeGenerator(fncg).generate(superType_, arguments_, name_);
}

}  // namespace EmojicodeCompiler
