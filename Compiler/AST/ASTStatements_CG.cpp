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
#include "Generation/Declarator.hpp"
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
ASTReturn::~ASTReturn() = default;
void ASTReturn::addRelease(std::unique_ptr<ASTRelease> release) { releases_.emplace_back(std::move(release)); }

void ASTReturn::release(FunctionCodeGenerator *fg) const {
    fg->releaseTemporaryObjects();

    for (auto &release : releases_) {
        release->generate(fg);
    }
}

void ASTReturn::generate(FunctionCodeGenerator *fg) const {
    if (value_) {
        auto val = value_->generate(fg);
        release(fg);
        fg->builder().CreateRet(val);
    }
    else {
        release(fg);
        fg->builder().CreateRetVoid();
    }
}

void ASTRaise::generate(FunctionCodeGenerator *fg) const {
    if (boxed_) {
        auto box = fg->createEntryAlloca(fg->typeHelper().box());
        fg->buildMakeNoValue(box);
        auto ptr = fg->buildGetBoxValuePtr(box, value_->expressionType());
        fg->builder().CreateStore(value_->generate(fg), ptr);
        auto val = fg->builder().CreateLoad(box);
        release(fg);
        fg->builder().CreateRet(val);
    }
    else {
        auto val = fg->buildSimpleErrorWithError(value_->generate(fg), fg->llvmReturnType());
        release(fg);
        buildDestruct(fg);
        fg->builder().CreateRet(val);
    }
}

}  // namespace EmojicodeCompiler
