//
//  ASTStatements_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Scoping/CGScoper.hpp"

namespace EmojicodeCompiler {

void ASTBlock::generate(FunctionCodeGenerator *fg) const {
    for (auto &stmt : stmts_) {
        stmt->generate(fg);
    }
}

void ASTReturn::generate(FunctionCodeGenerator *fg) const {
    if (value_) {
        fg->builder().CreateRet(value_->generate(fg));
    }
    else {
        fg->builder().CreateRetVoid();
    }
}

void ASTRaise::generate(FunctionCodeGenerator *fg) const {
    // TODO: implement
}


}  // namespace EmojicodeCompiler
