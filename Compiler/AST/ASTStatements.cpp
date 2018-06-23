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
    for (size_t i = 0; i < stmts_.size(); i++) {
        if (!returnedCertainly_  && analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::Returned)) {
            returnedCertainly_ = true;
            stop_ = i;
            analyser->compiler()->warn(stmts_[i]->position(), "Code will never be executed.");
        }
        stmts_[i]->analyse(analyser);
    }

    if (!returnedCertainly_  && analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::Returned)) {
        returnedCertainly_ = true;
        stop_ = stmts_.size();
    }
}

void ASTExprStatement::analyse(FunctionAnalyser *analyser)  {
    expr_->setExpressionType(expr_->analyse(analyser, TypeExpectation()));
}

void ASTReturn::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);

    if (analyser->function()->returnType()->type().type() == TypeType::NoReturn) {
        assert(value_ == nullptr);
        return;
    }

    if (value_ == nullptr) {
        throw CompilerError(position(), "↩️↩️ can only be used in functions without a return value.");
    }

    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "↩️ cannot be used inside an initializer.");
    }

    analyser->expectType(analyser->function()->returnType()->type(), &value_);
}

void ASTRaise::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        auto *initializer = dynamic_cast<Initializer *>(analyser->function());
        if (!initializer->errorProne()) {
            throw CompilerError(position(), "Initializer is not declared error-prone.");
        }
        analyser->expectType(initializer->errorType()->type(), &value_);
        return;
    }

    if (analyser->function()->returnType()->type().unboxedType() != TypeType::Error) {
        throw CompilerError(position(), "Function is not declared to return a 🚨.");
    }

    boxed_ = analyser->function()->returnType()->type().storageType() == StorageType::Box;

    analyser->expectType(analyser->function()->returnType()->type().errorEnum(), &value_);
}

}  // namespace EmojicodeCompiler
