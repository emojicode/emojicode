//
//  ASTTypeExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "ASTLiterals.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Types/TypeExpectation.hpp"
#include "CompilerError.hpp"

namespace EmojicodeCompiler {

Type ASTInferType::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeType::StorageExpectation || expectation.type() == TypeType::NoReturn) {
        throw CompilerError(position(), "Cannot infer ⚫️.");
    }
    type_ = std::make_unique<ASTLiteralType>(expectation.copyType().unoptionalized());
    type_->type().setExact(true);
    return type_->type();
}

Type ASTTypeFromExpr::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto value = analyser->expect(expectation, &expr_);
    if (value.type() != TypeType::TypeAsValue) {
        throw CompilerError(position(), "Expected type value.");
    }
    return value.typeOfTypeValue();
}

Type ASTStaticType::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    type_->analyseType(analyser->typeContext()).setExact(true);
    if (type_->type().type() == TypeType::GenericVariable || type_->type().type() == TypeType::LocalGenericVariable) {
        throw CompilerError(position(), "Generic Arguments are not available dynamically.");
    }
    return type_->type();
}

ASTThisType::ASTThisType(const SourcePosition &p) : ASTTypeFromExpr(std::make_shared<ASTThis>(p), p) {}

}  // namespace EmojicodeCompiler
