//
//  ASTLiterals_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTLiterals.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Generation/StringPool.hpp"
#include "../Application.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

void ASTStringLiteral::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_STRING_POOL);
    fncg->wr().writeInstruction(fncg->app()->stringPool().pool(value_));
}

void ASTBooleanTrue::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_TRUE);
}

void ASTBooleanFalse::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_FALSE);
}

void ASTNumberLiteral::generateExpr(FnCodeGenerator *fncg) const {
    switch (type_) {
        case NumberType::Integer:
            fncg->writeInteger(integerValue_);
            break;
        case NumberType::Double:
            fncg->wr().writeInstruction(INS_GET_DOUBLE);
            fncg->wr().writeDoubleCoin(doubleValue_);
            break;
    }
}

void ASTSymbolLiteral::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_SYMBOL);
    fncg->wr().writeInstruction(value_);
}

void ASTThis::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_THIS);
}

void ASTNothingness::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_NOTHINGNESS);
    fncg->wr().writeInstruction(INS_PUSH_N);
    fncg->wr().writeInstruction(type_.size() - 1);
}

void ASTDictionaryLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(varId_, type_);
    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    });
    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
                                                                   std::u32string(1, 0x1F438));
    fncg->copyToVariable(var.stackIndex, false, type_);

    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
        fncg->pushVariable(var.stackIndex, false, var.type);
    });
    for (auto it = values_.begin(); it != values_.end(); it++) {
        auto args = ASTArguments(position());
        args.addArguments(*it);
        args.addArguments(*(++it));
        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F437));
    }
    getVar.generateExpr(fncg);
}

void ASTListLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(varId_, type_);
    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    });
    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
                                                                   std::u32string(1, 0x1F438));
    fncg->copyToVariable(var.stackIndex, false, type_);

    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
        fncg->pushVariable(var.stackIndex, false, var.type);
    });
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F43B));
    }
    getVar.generateExpr(fncg);
}

void ASTConcatenateLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(varId_, type_);
    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    });
    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
                                                                   std::u32string(1, 0x1F195));
    fncg->copyToVariable(var.stackIndex, false, type_);

    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
        fncg->pushVariable(var.stackIndex, false, var.type);
    });
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F43B));
    }

    CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_,
                                                          ASTArguments(position()), std::u32string(1, 0x1F521));
}

}  // namespace EmojicodeCompiler
