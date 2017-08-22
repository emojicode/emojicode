//
//  ASTLiterals.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTLiterals.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Generation/StringPool.hpp"
#include "../Types/TypeExpectation.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

Type ASTStringLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type(CL_STRING, false);
}

void ASTStringLiteral::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_STRING_POOL);
    fncg->wr().writeInstruction(StringPool::theStringPool().poolString(value_));
}

Type ASTBooleanTrue::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::boolean();
}

void ASTBooleanTrue::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_TRUE);
}

Type ASTBooleanFalse::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::boolean();
}

void ASTBooleanFalse::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_FALSE);
}

Type ASTNumberLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeType::ValueType && expectation.valueType() == VT_DOUBLE
        && type_ == NumberType::Integer) {
        type_ = NumberType::Double;
        doubleValue_ = integerValue_;
    }

    switch (type_) {
        case NumberType::Integer:
            return Type::integer();
        case NumberType::Double:
            return Type::doubl();
    }
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

Type ASTSymbolLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::symbol();
}

void ASTSymbolLiteral::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_SYMBOL);
    fncg->wr().writeInstruction(value_);
}

Type ASTThis::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (isSuperconstructorRequired(analyser->function()->functionType()) &&
        !analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::CalledSuperInitializer) &&
        analyser->typeContext().calleeType().eclass()->superclass() != nullptr) {
        throw CompilerError(position(), "Attempt to use 🐕 before superinitializer call.");
    }
    if (isFullyInitializedCheckRequired(analyser->function()->functionType())) {
        analyser->scoper().instanceScope()->initializerUnintializedVariablesCheck(position(),
                                                                       "Instance variable \"%s\" must be "
                                                                       "initialized before the use of 🐕.");
    }

    if (!isSelfAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "Illegal use of 🐕.");
    }
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    return analyser->typeContext().calleeType();
}

void ASTThis::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_THIS);
}

Type ASTNothingness::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::nothingness();
}

void ASTNothingness::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_NOTHINGNESS);
}

Type ASTDictionaryLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type(CL_DICTIONARY, false);  // TODO: infer and transform
}

void ASTDictionaryLiteral::generateExpr(FnCodeGenerator *fncg) const {
    // TODO: implement
}

Type ASTListLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = Type(CL_LIST, false);

    analyser->scoper().pushTemporaryScope();
    auto variable = analyser->scoper().currentScope().declareInternalVariable(type_, position());
    varId_ = variable.id();

    CommonTypeFinder finder;
    for (auto &valueNode : values_) {
        Type type = analyser->expect(TypeExpectation(false, true, false), &valueNode);
        finder.addType(type, analyser->typeContext());
    }

    type_.setGenericArgument(0, finder.getCommonType(position()));
    return type_;
}

void ASTListLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(varId_, type_);
    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    });
    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, ASTArguments(position()), EmojicodeString(0x1F438));
    fncg->copyToVariable(var.stackIndex, false, type_);

    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
        fncg->pushVariable(var.stackIndex, false, var.type);
    });
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, args, EmojicodeString(0x1F43B));
    }
    getVar.generateExpr(fncg);
}

Type ASTConcatenateLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    analyser->function()->package()->fetchRawType(EmojicodeString(0x1F520), kDefaultNamespace,
                                                  false, position(), &type_);

    analyser->scoper().pushTemporaryScope();
    auto variable = analyser->scoper().currentScope().declareInternalVariable(type_, position());
    varId_ = variable.id();

    for (auto &stringNode : values_) {
        analyser->expectType(Type(CL_STRING, false), &stringNode);
    }
    return Type(CL_STRING, false);
}

void ASTConcatenateLiteral::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = fncg->scoper().declareVariable(varId_, type_);
    auto type = ASTProxyExpr(position(), type_, [this](auto *fncg) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    });
    InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(type, ASTArguments(position()), EmojicodeString(0x1F195));
    fncg->copyToVariable(var.stackIndex, false, type_);

    auto getVar = ASTProxyExpr(position(), type_, [&var](auto *fncg) {
        fncg->pushVariable(var.stackIndex, false, var.type);
    });
    for (auto &stringNode : values_) {
        auto args = ASTArguments(position());
        args.addArguments(stringNode);
        CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, args, EmojicodeString(0x1F43B));
    }

    CallCodeGenerator(fncg, INS_DISPATCH_METHOD).generate(getVar, ASTArguments(position()), EmojicodeString(0x1F521));
}

}  // namespace EmojicodeCompiler
