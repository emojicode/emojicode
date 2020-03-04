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
#include "Scoping/SemanticScoper.hpp"
#include "Compiler.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

void ASTUnaryMFForwarding::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    expr_->analyseMemoryFlow(analyser, type);
}

Type ASTUnwrap::analyse(ExpressionAnalyser *analyser) {
    auto call = dynamic_cast<ASTCall *>(expr_.get());
    if (call != nullptr) {
        call->setHandledError();
    }

    Type t = analyser->expect(TypeExpectation(false, false), &expr_);

    if (t.unboxedType() == TypeType::Optional) {
        return t.optionalType();
    }
    if (call != nullptr && call->isErrorProne()) {
        error_ = true;
        auto error = analyser->compiler()->sError;
        method_ = error->methods().lookup(U"🤯", Mood::Imperative, { Type(analyser->compiler()->sString) }, Type(error),
                                          TypeContext(), analyser->semanticAnalyser());
        return t;
    }

    throw CompilerError(position(), "🍺 can only be used with optionals or error-prone calls.");
}

Type ASTReraise::analyse(ExpressionAnalyser *analyser) {
    auto call = dynamic_cast<ASTCall *>(expr_.get());
    if (call != nullptr) call->setHandledError();
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (call == nullptr || !call->isErrorProne()) {
        analyser->error(CompilerError(position(), "Provided value is not an error-prone call."));
        return t;
    }
    auto fa = dynamic_cast<FunctionAnalyser *>(analyser);
    if (fa == nullptr) {
        analyser->error(CompilerError(position(), "Not in a function context."));
        return t;
    }
    auto fta = fa->function()->errorType()->type();
    if (!call->errorType().compatibleTo(fta, analyser->typeContext())) {
        analyser->error(CompilerError(position(), "Call may raise ",
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

Type ASTSelection::analyse(ExpressionAnalyser *analyser) {
    analyser->analyse(expr_);
    return analyser->analyseTypeExpr(typeExpr_, TypeExpectation());
}

Type ASTSelection::comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (!analyser->comply(TypeExpectation(expressionType()), &expr_)
            .compatibleTo(expressionType(), analyser->typeContext())) {
        analyser->compiler()->error(CompilerError(position(), "Expression cannot satisfy expectation."));
    }
    return expressionType().inexacted();
}

}  // namespace EmojicodeCompiler
