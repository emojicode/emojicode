//
//  ASTExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "Analysis/ExpressionAnalyser.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/TypeExpectation.hpp"
#include "CompilerError.hpp"
#include "ASTType.hpp"

namespace EmojicodeCompiler {

ASTSizeOf::ASTSizeOf(std::unique_ptr<ASTType> type, const SourcePosition &p) : ASTExpr(p), type_(std::move(type)) {}

Type ASTSizeOf::analyse(ExpressionAnalyser *analyser) {
    type_->analyseType(analyser->typeContext());
    return analyser->integer();
}

ASTSizeOf::~ASTSizeOf() = default;

Type ASTCallableCall::analyse(ExpressionAnalyser *analyser) {
    Type type = analyser->expect(TypeExpectation(false, false), &callable_);
    if (type.type() != TypeType::Callable) {
        throw CompilerError(position(), "Given value is not callable.");
    }
    if (args_.args().size() != type.parametersCount()) {
        throw CompilerError(position(), "Callable expects ", type.parametersCount(),
                            " arguments but ", args_.args().size(), " were supplied.");
    }
    for (size_t i = 0; i < type.parametersCount(); i++) {
        analyser->expectType(type.parameters()[i], &args_.args()[i]);
    }
    ensureErrorIsHandled(analyser);
    return type.returnType();
}

void ASTCallableCall::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    callable_->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    for (auto &arg : args_.args()) {
        arg->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);  // We cannot at all say what the callable will do.
    }
}

void ASTCall::ensureErrorIsHandled(ExpressionAnalyser *analyser) const {
    if (isErrorProne() && !handledError_) {
        analyser->error(CompilerError(position(), "Call can raise ", errorType().toString(analyser->typeContext()),
                                      " but error is not handled."));
    }
}

ASTArguments::ASTArguments(const SourcePosition &p) : ASTNode(p) {}
ASTArguments::ASTArguments(const SourcePosition &p, std::vector<std::shared_ptr<ASTExpr>> args)
        : ASTNode(p), arguments_(std::move(args)) {}
ASTArguments::ASTArguments(const SourcePosition &p, Mood mood) : ASTNode(p), mood_(mood) {}

void ASTArguments::addGenericArgument(std::unique_ptr<ASTType> type) {
    genericArguments_.emplace_back(std::move(type));
}

ASTArguments::~ASTArguments() = default;

}  // namespace EmojicodeCompiler
