//
//  ASTStatements.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTStatements.hpp"
#include "../../EmojicodeInstructions.h"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../FunctionType.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Scoping/CGScoper.hpp"
#include "../Scoping/VariableNotFoundError.hpp"

namespace EmojicodeCompiler {

void ASTBlock::analyse(SemanticAnalyser *analyser) {
    for (auto &stmt : stmts_) {
        stmt->analyse(analyser);
    }
}

void ASTBlock::generate(FnCodeGenerator *fncg) const {
    for (auto &stmt : stmts_) {
        stmt->generate(fncg);
    }
}

void ASTExprStatement::analyse(SemanticAnalyser *analyser)  {
    expr_->analyse(analyser, TypeExpectation());
    analyser->popTemporaryScope(expr_);
}

void ASTReturn::analyse(SemanticAnalyser *analyser) {
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::Returned);
    if (analyser->function()->returnType.type() == TypeContent::Nothingness) {
        return;
    }

    if (isOnlyNothingnessReturnAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "🍎 cannot be used inside an initializer.");
    }

    analyser->expectType(analyser->function()->returnType, &value_);
}

void ASTReturn::generate(FnCodeGenerator *fncg) const {
    if (value_) {
        value_->generate(fncg);
    }
    fncg->wr().writeInstruction(INS_RETURN);
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

    if (analyser->function()->returnType.type() != TypeContent::Error) {
        throw CompilerError(position(), "Function is not declared to return a 🚨.");
    }

    boxed_ = analyser->function()->returnType.storageType() == StorageType::Box;

    analyser->expectType(analyser->function()->returnType.genericArguments()[0], &value_);
}

void ASTRaise::generate(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_PUSH_ERROR);
    value_->generate(fncg);
    if (boxed_) {
        fncg->wr().writeInstruction(INS_PUSH_N),
        fncg->wr().writeInstruction(kBoxValueSize - value_->expressionType().size() - 1);
    }
    fncg->wr().writeInstruction(INS_RETURN);
}

void ASTSuperinitializer::analyse(SemanticAnalyser *analyser) {
    if (!isSuperconstructorRequired(analyser->function()->functionType())) {
        throw CompilerError(position(), "🐐 can only be used inside initializers.");
    }
    if (analyser->typeContext().calleeType().eclass()->superclass() == nullptr) {
        throw CompilerError(position(), "🐐 can only be used if the class inherits from another.");
    }
    if (analyser->pathAnalyser().hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
        throw CompilerError(position(), "Superinitializer might have already been called.");
    }

    analyser->scoper().instanceScope()->initializerUnintializedVariablesCheck(position(),
                                                                   "Instance variable \"%s\" must be "
                                                                   "initialized before superinitializer.");

    Class *eclass = analyser->typeContext().calleeType().eclass();
    auto initializer = eclass->superclass()->getInitializer(name_, Type(eclass, false),
                                                            analyser->typeContext(), position());
    superType_ = Type(eclass->superclass(), false);
    analyser->analyseFunctionCall(&arguments_, superType_, initializer);

    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::CalledSuperInitializer);
}

void ASTSuperinitializer::generate(FnCodeGenerator *fncg) const {
    SuperInitializerCallCodeGenerator(fncg, INS_SUPER_INITIALIZER).generate(superType_, arguments_, name_);
    fncg->wr().writeInstruction(INS_POP);
}

}  // namespace EmojicodeCompiler
