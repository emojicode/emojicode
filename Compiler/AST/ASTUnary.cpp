//
//  ASTUnary.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "CompilerError.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTIsError::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false), &value_);
    if (type.unboxedType() != TypeType::Error) {
        throw CompilerError(position(), "🚥 can only be used with 🚨.");
    }
    return analyser->boolean();
}

Type ASTUnwrap::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(false, false), &value_);

    if (t.unboxedType() == TypeType::Optional) {
        return t.optionalType();
    }
    if (t.unboxedType() == TypeType::Error) {
        error_ = true;
        return t.errorType();
    }

    throw CompilerError(position(), "🍺 can only be used with optionals or 🚨.");
}

}  // namespace EmojicodeCompiler
