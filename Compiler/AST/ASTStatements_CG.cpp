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
#include "Scoping/IDScoper.hpp"

namespace EmojicodeCompiler {

void ASTBlock::generate(FunctionCodeGenerator *fg) const {
    auto stop = !returnedCertainly_ ? stmts_.size() : stop_;
    for (size_t i = 0; i < stop; i++) {
        stmts_[i]->generate(fg);
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
    if (boxed_) {
        auto box = fg->createEntryAlloca(fg->typeHelper().box());
        fg->getMakeNoValue(box);
        auto ptr = fg->getValuePtr(box, value_->expressionType());
        fg->builder().CreateStore(value_->generate(fg), ptr);
        fg->builder().CreateRet(fg->builder().CreateLoad(box));
    }
    else {
        fg->builder().CreateRet(fg->getSimpleErrorWithError(value_->generate(fg), fg->llvmReturnType()));
    }
}

}  // namespace EmojicodeCompiler
