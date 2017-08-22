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
    if (!type.optional() && type.type() != TypeContent::Something) {
        throw CompilerError(position(), "☁️ can only be used with optionals and ⚪️.");
    }
    return Type::boolean();
}

void ASTIsNothigness::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_IS_NOTHINGNESS);
}

Type ASTIsError::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(true, false), &value_);
    if (type.type() != TypeContent::Error) {
        throw CompilerError(position(), "🚥 can only be used with 🚨.");
    }
    return Type::boolean();
}

void ASTIsError::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_IS_ERROR);
}

Type ASTUnwrap::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(true, false), &value_);
    assert(t.isReference());

    t.setReference(false);

    if (t.optional()) {
        t.setOptional(false);
        return t;
    }
    if (t.type() == TypeContent::Error) {
        error_ = true;
        return t.genericArguments()[1];
    }

    throw CompilerError(position(), "🍺 can only be used with optionals or 🚨.");
}

void ASTUnwrap::generateExpr(FnCodeGenerator *fncg) const {
    value_->generate(fncg);
    auto type = value_->expressionType();
    if (type.storageType() == StorageType::Box) {
        fncg->wr().writeInstruction(error_ ? INS_ERROR_CHECK_BOX_OPTIONAL : INS_UNWRAP_BOX_OPTIONAL);
    }
    else {
        fncg->wr().writeInstruction(error_ ? INS_ERROR_CHECK_SIMPLE_OPTIONAL : INS_UNWRAP_SIMPLE_OPTIONAL);
        fncg->wr().writeInstruction(type.size() - 1);
    }
}

Type ASTMetaTypeFromInstance::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type originalType = analyser->expect(TypeExpectation(false, false, false), &value_);
    analyser->validateMetability(originalType, position());
    originalType.setMeta(true);
    return originalType;
}

void ASTMetaTypeFromInstance::generateExpr(FnCodeGenerator *fncg) const {
    generateHelper(fncg, INS_GET_CLASS_FROM_INSTANCE);
}

}  // namespace EmojicodeCompiler
