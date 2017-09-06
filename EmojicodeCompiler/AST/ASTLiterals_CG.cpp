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
#include "../Application.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

Value* ASTStringLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto data = llvm::ArrayRef<uint32_t>(reinterpret_cast<const uint32_t *>(value_.data()), value_.size());
    return llvm::ConstantDataArray::get(fncg->generator()->context(), data);
}

Value* ASTBooleanTrue::generateExpr(FnCodeGenerator *fncg) const {
    return llvm::ConstantInt::getTrue(fncg->generator()->context());
}

Value* ASTBooleanFalse::generateExpr(FnCodeGenerator *fncg) const {
    return llvm::ConstantInt::getFalse(fncg->generator()->context());
}

Value* ASTNumberLiteral::generateExpr(FnCodeGenerator *fncg) const {
    switch (type_) {
        case NumberType::Integer:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(fncg->generator()->context()), integerValue_);
        case NumberType::Double:
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(fncg->generator()->context()), doubleValue_);
    }
}

Value* ASTSymbolLiteral::generateExpr(FnCodeGenerator *fncg) const {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(fncg->generator()->context()), value_);
}

Value* ASTThis::generateExpr(FnCodeGenerator *fncg) const {
    return fncg->thisValue();
}

Value* ASTNothingness::generateExpr(FnCodeGenerator *fncg) const {
    auto structType = fncg->typeHelper().llvmTypeFor(type_);
    auto undef = llvm::UndefValue::get(structType);
    return fncg->builder().CreateInsertValue(undef, fncg->generator()->optionalNoValue(), 0);
}

Value* ASTDictionaryLiteral::generateExpr(FnCodeGenerator *fncg) const {
//    auto &var = fncg->scoper().declareVariable(varId_, type_);
//    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
//        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
//        fncg->wr().writeInstruction(type_.eclass()->index);
//    });
//    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
//                                                                   std::u32string(1, 0x1F438));
//    fncg->copyToVariable(var.stackIndex, false, type_);
//
//    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
//        fncg->pushVariable(var.stackIndex, false, var.type);
//    });
//    for (auto it = values_.begin(); it != values_.end(); it++) {
//        auto args = ASTArguments(position());
//        args.addArguments(*it);
//        args.addArguments(*(++it));
//        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F437));
//    }
//    getVar.generateExpr(fncg);
    return nullptr;
}

Value* ASTListLiteral::generateExpr(FnCodeGenerator *fncg) const {
//    auto &var = fncg->scoper().declareVariable(varId_, type_);
//    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
//        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
//        fncg->wr().writeInstruction(type_.eclass()->index);
//    });
//    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
//                                                                   std::u32string(1, 0x1F438));
//    fncg->copyToVariable(var.stackIndex, false, type_);
//
//    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
//        fncg->pushVariable(var.stackIndex, false, var.type);
//    });
//    for (auto &stringNode : values_) {
//        auto args = ASTArguments(position());
//        args.addArguments(stringNode);
//        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F43B));
//    }
//    getVar.generateExpr(fncg);
    return nullptr;
}

Value* ASTConcatenateLiteral::generateExpr(FnCodeGenerator *fncg) const {
//    auto &var = fncg->scoper().declareVariable(varId_, type_);
//    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
//        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
//        fncg->wr().writeInstruction(type_.eclass()->index);
//    });
//    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, type_, ASTArguments(position()),
//                                                                   std::u32string(1, 0x1F195));
//    fncg->copyToVariable(var.stackIndex, false, type_);
//
//    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
//        fncg->pushVariable(var.stackIndex, false, var.type);
//    });
//    for (auto &stringNode : values_) {
//        auto args = ASTArguments(position());
//        args.addArguments(stringNode);
//        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_, args, std::u32string(1, 0x1F43B));
//    }
//
//    CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, type_,
//                                                          ASTArguments(position()), std::u32string(1, 0x1F521));
    return nullptr;
}

}  // namespace EmojicodeCompiler
