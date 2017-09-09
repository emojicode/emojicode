//
//  ASTStatements.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Application.hpp"
#include "../Functions/FunctionType.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Class.hpp"
#include "../Scoping/VariableNotFoundError.hpp"

namespace EmojicodeCompiler {

void ASTBlock::analyse(SemanticAnalyser *analyser) {
    for (auto &stmt : stmts_) {
        stmt->analyse(analyser);
    }
    returnedCertainly_ = analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::Returned);
}

void ASTExprStatement::analyse(SemanticAnalyser *analyser)  {
    expr_->analyse(analyser, TypeExpectation());
}

void ASTReturn::analyse(SemanticAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (analyser->function()->returnType.type() == TypeType::NoReturn) {
        return;
    }

    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "🍎 cannot be used inside an initializer.");
    }

    analyser->expectType(analyser->function()->returnType, &value_);
}

void ASTRaise::analyse(SemanticAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        auto *initializer = dynamic_cast<Initializer *>(analyser->function());
        if (!initializer->errorProne()) {
            throw CompilerError(position(), "Initializer is not declared error-prone.");
        }
        analyser->expectType(initializer->errorType(), &value_);
        return;
    }

    if (analyser->function()->returnType.type() != TypeType::Error) {
        throw CompilerError(position(), "Function is not declared to return a 🚨.");
    }

    boxed_ = analyser->function()->returnType.storageType() == StorageType::Box;

    analyser->expectType(analyser->function()->returnType.genericArguments()[0], &value_);
}

void ASTSuperinitializer::analyse(SemanticAnalyser *analyser) {
    if (!isSuperconstructorRequired(analyser->function()->functionType())) {
        throw CompilerError(position(), "🐐 can only be used inside initializers.");
    }
    if (analyser->typeContext().calleeType().eclass()->superclass() == nullptr) {
        throw CompilerError(position(), "🐐 can only be used if the class inherits from another.");
    }
    if (analyser->pathAnalyser().hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
        analyser->app()->error(CompilerError(position(), "Superinitializer might have already been called."));
    }

    analyser->scoper().instanceScope()->unintializedVariablesCheck(position(), "Instance variable \"", "\" must be "
                                                                   "initialized before calling the superinitializer.");

    Class *eclass = analyser->typeContext().calleeType().eclass();
    auto initializer = eclass->superclass()->getInitializer(name_, Type(eclass, false),
                                                            analyser->typeContext(), position());
    superType_ = Type(eclass->superclass(), false);
    analyser->analyseFunctionCall(&arguments_, superType_, initializer);

    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::CalledSuperInitializer);
}

}  // namespace EmojicodeCompiler
