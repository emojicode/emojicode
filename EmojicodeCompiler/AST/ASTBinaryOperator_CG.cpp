//
//  ASTBinaryOperator_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTBinaryOperator.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTBinaryOperator::generateExpr(FnCodeGenerator *fncg) const {
    if (builtIn_) {
        left_->generate(fncg);
        right_->generate(fncg);
        fncg->wr().writeInstruction(instruction_);
        return nullptr;
    }

    CallCodeGenerator(fncg, instruction_).generate(*left_, calleeType_, args_, operatorName(operator_));
    return nullptr;
}

}  // namespace EmojicodeCompiler
