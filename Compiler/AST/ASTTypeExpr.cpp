//
//  ASTTypeExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "ASTType.hpp"
#include "ASTLiterals.hpp"
#include "Analysis/ExpressionAnalyser.hpp"
#include "CompilerError.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

ASTStaticType::ASTStaticType(std::unique_ptr<ASTType> type, const SourcePosition &p)
    : ASTTypeExpr(p), type_(std::move(type)) {}

Type ASTInferType::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation, bool) {
    if (expectation.type() == TypeType::StorageExpectation || expectation.type() == TypeType::NoReturn) {
        throw CompilerError(position(), "Cannot infer ⚫️.");
    }
    type_ = std::make_unique<ASTLiteralType>(expectation.copyType().unoptionalized());
    type_->type().setExact(true);
    return type_->type();
}

Type ASTTypeFromExpr::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation, bool) {
    auto value = analyser->expect(expectation, &expr_);
    if (value.type() != TypeType::TypeAsValue) {
        throw CompilerError(position(), "Expected type value.");
    }
    return value.typeOfTypeValue();
}

Type ASTStaticType::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation, bool allowGenericInference) {
    type_->analyseType(analyser->typeContext(), false, allowGenericInference).setExact(true);
    if (type_->type().type() == TypeType::GenericVariable || type_->type().type() == TypeType::LocalGenericVariable) {
        throw CompilerError(position(), "Generic Arguments are not available dynamically.");
    }
    return type_->type();
}

Type ASTTypeExpr::analyse(ExpressionAnalyser *analyser) {
    return analyse(analyser, TypeExpectation(), false);
}

ASTThisType::ASTThisType(const SourcePosition &p) : ASTTypeFromExpr(std::make_shared<ASTThis>(p), p) {}

ASTStaticType::~ASTStaticType() = default;

}  // namespace EmojicodeCompiler
