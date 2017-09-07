//
//  ASTStatements_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"
#include "../Scoping/CGScoper.hpp"

namespace EmojicodeCompiler {

void ASTBlock::generate(FunctionCodeGenerator *fncg) const {
    for (auto &stmt : stmts_) {
        stmt->generate(fncg);
    }
}

void ASTReturn::generate(FunctionCodeGenerator *fncg) const {
    if (value_) {
        fncg->builder().CreateRet(value_->generate(fncg));
    }
    else {
        fncg->builder().CreateRetVoid();
    }
}

void ASTRaise::generate(FunctionCodeGenerator *fncg) const {
    // TODO: implement
}

void ASTSuperinitializer::generate(FunctionCodeGenerator *fncg) const {
    auto castedThis = fncg->builder().CreateBitCast(fncg->thisValue(), fncg->typeHelper().llvmTypeFor(superType_));
    InitializationCallCodeGenerator(fncg, CallType::StaticDispatch).generate(castedThis, superType_, arguments_, name_);
}

}  // namespace EmojicodeCompiler
