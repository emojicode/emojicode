//
//  ASTLiterals.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Generation/StringPool.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/RunTimeHelper.hpp"
#include "Generation/CallCodeGenerator.hpp"
#include "ASTInitialization.hpp"
#include "ASTLiterals.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Package/Package.hpp"
#include "Parsing/AbstractParser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTStringLiteral::analyse(ExpressionAnalyser *analyser) {
    auto type = Type(analyser->compiler()->sString);
    type.setExact(true);
    return type;
}

Type ASTBooleanTrue::analyse(ExpressionAnalyser *analyser) {
    return analyser->boolean();
}

Type ASTBooleanFalse::analyse(ExpressionAnalyser *analyser) {
    return analyser->boolean();
}

Type ASTNumberLiteral::analyse(ExpressionAnalyser *analyser) {
    if (type_ == NumberType::Integer) {
        return Type::integerLiteral();
    }
    return analyser->real();
}

Type ASTNumberLiteral::comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (type_ == NumberType::Double) {
        return analyser->real();
    }
    if (expectation == analyser->real()) {
        type_ = NumberType::Double;
        doubleValue_ = integerValue_;
        return analyser->real();
    }
    if (expectation == analyser->byte()) {
        if (integerValue_ > 255) {
            analyser->compiler()->warn(position(), "Literal implicitly is a byte integer literal but value does ",
                                       "not fit into byte type.");
        }
        type_ = NumberType::Byte;
        return analyser->byte();
    }
    return analyser->integer();
}

Type ASTThis::analyse(ExpressionAnalyser *analyser) {
    analyser->checkThisUse(position());

    if (!isSelfAllowed(analyser->functionType())) {
        throw CompilerError(position(), "Illegal use of 👇.");
    }
    analyser->pathAnalyser().record(PathAnalyserIncident::UsedSelf);
    return analyser->typeContext().calleeType();
}

void ASTThis::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->recordThis(type);
}

Type ASTNoValue::analyse(ExpressionAnalyser *analyser) {
    return Type::noValueLiteral();
}

Type ASTNoValue::comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.unboxedType() != TypeType::Optional && expectation.unboxedType() != TypeType::Something) {
        throw CompilerError(position(), "🤷‍ can only be used when an optional is expected.");
    }
    type_ = expectation.copyType();
    return type_;
}

ASTCollectionLiteral::ASTCollectionLiteral(const SourcePosition &p) : ASTExpr(p) {}

ASTCollectionLiteral::~ASTCollectionLiteral() = default;

Type ASTCollectionLiteral::analyse(ExpressionAnalyser *analyser) {
     finder_ = std::make_unique<CommonTypeFinder>(analyser->semanticAnalyser());

    if (pairs_) {
        for (auto it = values_.begin(); it != values_.end(); it++) {
            analyser->analyse(*it);
            if (++it == values_.end()) {
                throw CompilerError(position(), "A value must be provided for every key.");
            }
            finder_->addType(analyser->analyse(*it), analyser->typeContext());
        }
        return Type::dictionaryLiteral(finder_->getCommonType());
    }

    for (auto &valueNode : values_) {
        finder_->addType(analyser->analyse(valueNode), analyser->typeContext());
    }
    return Type::listLiteral(finder_->getCommonType());
}

Type ASTCollectionLiteral::complyPairs(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeType::ValueType && expectation.typeDefinition()->canInitFrom(expressionType())) {
        type_ = expectation.copyType();
    }
    else {
        finder_->issueWarning(position(), analyser->compiler());
        type_ = analyser->semanticAnalyser()->defaultLiteralType(expressionType());
    }
    finder_ = nullptr;
    type_.setExact(true);

    Type elementType = analyser->compiler()->sDictionary->typeForVariable(0).resolveOn(TypeContext(type_));
    for (auto it = values_.begin(); it != values_.end(); it++) {
        analyser->comply(TypeExpectation(analyser->compiler()->sString->type()), &(*it));
        if (++it == values_.end()) {
            throw CompilerError(position(), "A value must be provided for every key.");
        }
        analyser->comply(TypeExpectation(elementType), &(*it));
    }
    initializer_ = type_.typeDefinition()->inits().lookup(U"🍪", Mood::Imperative,
                                                          { analyser->compiler()->sMemory->type(), analyser->compiler()->sMemory->type(), analyser->integer() }, type_, analyser->typeContext(),
                                                          analyser->semanticAnalyser());
    initializer_->createUnspecificReification();
    return type_;
}

void ASTCollectionLiteral::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    for (auto &valueNode : values_) {
        valueNode->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);
    }
}

Type ASTCollectionLiteral::comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (pairs_) return complyPairs(analyser, expectation);
    if (expectation.type() == TypeType::ValueType && expectation.typeDefinition()->canInitFrom(expressionType())) {
        type_ = expectation.copyType();
    }
    else {
        finder_->issueWarning(position(), analyser->compiler());
        type_ = analyser->semanticAnalyser()->defaultLiteralType(expressionType());
    }
    finder_ = nullptr;
    type_.setExact(true);

    Type elementType = analyser->compiler()->sList->typeForVariable(0).resolveOn(TypeContext(type_));
    for (auto &valueNode : values_) {
        analyser->comply(TypeExpectation(elementType), &valueNode);
    }
    initializer_ = type_.typeDefinition()->inits().lookup(U"🍪", Mood::Imperative,
            { analyser->compiler()->sMemory->type(), analyser->integer() }, type_, analyser->typeContext(),
            analyser->semanticAnalyser());
    initializer_->createUnspecificReification();
    return type_;
}

Type ASTInterpolationLiteral::analyse(ExpressionAnalyser *analyser) {
    Type sb = analyser->package()->getRawType(TypeIdentifier(U"🔠", kDefaultNamespace, position()));
    init_ = sb.typeDefinition()->inits().lookup(U"🆕", Mood::Imperative, { Type(analyser->compiler()->sInteger) },
                                                Type(sb), analyser->typeContext(), analyser->semanticAnalyser());

    append_ = sb.typeDefinition()->methods().lookup(U"🐻", Mood::Imperative,
                                                    {Type(analyser->compiler()->sString)}, Type(sb),
                                                    analyser->typeContext(), analyser->semanticAnalyser());

    get_ = sb.typeDefinition()->methods().lookup(U"🔡", Mood::Imperative, {}, Type(sb),
                                                 analyser->typeContext(), analyser->semanticAnalyser());

    auto magnet = Type(analyser->compiler()->sInterpolateable).applyMinimalBoxing().referenced();
    toString_ = magnet.typeDefinition()->methods().lookup(U"🔡", Mood::Imperative, {}, magnet,
                                                          analyser->typeContext(), analyser->semanticAnalyser());

    for (auto &value : values_) {
        analyser->expectType(magnet, &value);
    }
    return analyser->compiler()->sString->type();
}

void ASTInterpolationLiteral::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    for (auto &valueNode : values_) {
        valueNode->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    }
}

}  // namespace EmojicodeCompiler
