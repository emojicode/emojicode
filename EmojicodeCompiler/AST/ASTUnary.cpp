//
//  ASTUnary.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTUnary.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../CompilerError.hpp"
#include "../Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTIsNothigness::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(true, false), &value_);
    if (!type.optional() && type.type() != TypeType::Something) {
        throw CompilerError(position(), "☁️ can only be used with optionals and ⚪️.");
    }
    return Type::boolean();
}

Type ASTIsError::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(true, false), &value_);
    if (type.type() != TypeType::Error) {
        throw CompilerError(position(), "🚥 can only be used with 🚨.");
    }
    return Type::boolean();
}

Type ASTUnwrap::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(true, false), &value_);
    assert(t.isReference());

    t.setReference(false);

    if (t.optional()) {
        t.setOptional(false);
        return t;
    }
    if (t.type() == TypeType::Error) {
        error_ = true;
        return t.genericArguments()[1];
    }

    throw CompilerError(position(), "🍺 can only be used with optionals or 🚨.");
}

Type ASTMetaTypeFromInstance::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type originalType = analyser->expect(TypeExpectation(false, false, false), &value_);
    analyser->validateMetability(originalType, position());
    originalType.setMeta(true);
    return originalType;
}

}  // namespace EmojicodeCompiler
