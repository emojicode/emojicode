//
//  ASTMethod_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "../Generation/CallCodeGenerator.hpp"

namespace EmojicodeCompiler {

Value* ASTMethod::generateExpr(FnCodeGenerator *fncg) const {
    if (builtIn_) {
        callee_->generate(fncg);
        fncg->wr().writeInstruction(instruction_);
        return nullptr;
    }
    CallCodeGenerator(fncg, instruction_).generate(*callee_, calleeType_,  args_, name_);
    return nullptr;
}
    
}  // namespace EmojicodeCompiler
