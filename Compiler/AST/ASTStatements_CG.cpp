//
//  ASTStatements_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "AST/ASTMemory.hpp"
#include "ASTStatements.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "Generation/RunTimeHelper.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Scoping/IDScoper.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

void ASTBlock::generate(FunctionCodeGenerator *fg) const {
    auto stop = !returnedCertainly_ ? stmts_.size() : stop_;
    for (size_t i = 0; i < stop; i++) {
        stmts_[i]->generate(fg);
        fg->releaseTemporaryObjects();
    }
}

ASTReturn::ASTReturn(std::shared_ptr<ASTExpr> value, const SourcePosition &p)
    : ASTStatement(p), value_(std::move(value)) {}

void ASTReturn::generate(FunctionCodeGenerator *fg) const {
    if (value_) {
        auto val = value_->generate(fg);
        fg->releaseTemporaryObjects();
        release(fg);
        fg->builder().CreateRet(val);
    }
    else {
        release(fg);
        fg->builder().CreateRetVoid();
    }
}

void ASTRaise::generate(FunctionCodeGenerator *fg) const {
    fg->builder().CreateStore(value_->generate(fg), fg->errorPointer());
    fg->releaseTemporaryObjects();
    release(fg);
    buildDestruct(fg);
    fg->buildErrorReturn();
}

}  // namespace EmojicodeCompiler
