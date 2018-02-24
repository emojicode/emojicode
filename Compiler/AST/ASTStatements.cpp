//
//  ASTStatements.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Functions/FunctionType.hpp"
#include "Functions/Initializer.hpp"
#include "Scoping/VariableNotFoundError.hpp"
#include "Types/Class.hpp"

namespace EmojicodeCompiler {

void ASTBlock::analyse(FunctionAnalyser *analyser) {
    for (auto &stmt : stmts_) {
        stmt->analyse(analyser);
    }
    returnedCertainly_ = analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::Returned);
}

void ASTExprStatement::analyse(FunctionAnalyser *analyser)  {
    expr_->setExpressionType(expr_->analyse(analyser, TypeExpectation()));
}

void ASTReturn::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (analyser->function()->returnType().type() == TypeType::NoReturn) {
        return;
    }

    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "↩️ cannot be used inside an initializer.");
    }

    analyser->expectType(analyser->function()->returnType(), &value_);
}

void ASTRaise::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        auto *initializer = dynamic_cast<Initializer *>(analyser->function());
        if (!initializer->errorProne()) {
            throw CompilerError(position(), "Initializer is not declared error-prone.");
        }
        analyser->expectType(initializer->errorType(), &value_);
        return;
    }

    if (analyser->function()->returnType().type() != TypeType::Error) {
        throw CompilerError(position(), "Function is not declared to return a 🚨.");
    }

    boxed_ = analyser->function()->returnType().storageType() == StorageType::Box;

    analyser->expectType(analyser->function()->returnType().genericArguments()[0], &value_);
}

}  // namespace EmojicodeCompiler
