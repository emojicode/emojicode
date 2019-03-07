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
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/TypeExpectation.hpp"
#include "Functions/Function.hpp"
#include "Compiler.hpp"
#include "Scoping/SemanticScoper.hpp"

namespace EmojicodeCompiler {

Type ASTUnwrap::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto call = dynamic_cast<ASTCall *>(expr_.get());
    if (call != nullptr) call->setHandledError();

    Type t = analyser->expect(TypeExpectation(false, false), &expr_);

    if (t.unboxedType() == TypeType::Optional) {
        return t.optionalType();
    }
    if (call != nullptr && call->isErrorProne()) {
        error_ = true;
        return t;
    }

    throw CompilerError(position(), "🍺 can only be used with optionals or error-prone calls.");
}

Type ASTReraise::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto call = dynamic_cast<ASTCall *>(expr_.get());
    if (call != nullptr) call->setHandledError();
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (call == nullptr || !call->isErrorProne()) {
        analyser->compiler()->error(CompilerError(position(), "Provided value is not an error-prone call."));
        return t;
    }
    auto fa = dynamic_cast<FunctionAnalyser *>(analyser);
    if (fa == nullptr) {
        analyser->compiler()->error(CompilerError(position(), "Not in a function context."));
        return t;
    }
    auto fta = fa->function()->errorType()->type();
    if (!call->errorType().compatibleTo(fta, analyser->typeContext())) {
        analyser->compiler()->error(CompilerError(position(), "Call may raise ",
                                                  call->errorType().toString(analyser->typeContext()),
                                                  " which cannot be reraised as it is not compatible to ",
                                                  fta.toString(analyser->typeContext()), "."));
    }
    stats_ = analyser->scoper().createStats();
    return t;
}

void ASTReraise::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->releaseAllVariables(this, stats_, position());
}

}  // namespace EmojicodeCompiler
