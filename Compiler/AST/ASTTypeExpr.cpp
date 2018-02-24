//
//  ASTTypeExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTInferType::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeType::StorageExpectation || expectation.type() == TypeType::NoReturn) {
        throw CompilerError(position(), "Cannot infer ⚫️.");
    }
    Type type = expectation.copyType();
    type.setOptional(false);
    type_ = type;
    availability_ = TypeAvailability::StaticAndAvailabale;
    return type;
}

Type ASTTypeFromExpr::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = expr_->analyse(analyser, expectation);
    if (!type.meta()) {
        throw CompilerError(position(), "Expected meta type.");
    }
    if (type.optional()) {
        throw CompilerError(position(), "🍬 can’t be used as meta type.");
    }
    type.setMeta(false);
    return type;
}

Type ASTStaticType::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    return type_;
}

Type ASTThisType::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    return analyser->typeContext().calleeType();
}

}  // namespace EmojicodeCompiler
