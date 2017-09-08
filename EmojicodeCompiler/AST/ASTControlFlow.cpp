//
//  ASTControlFlow.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTControlFlow.hpp"
#include "../Analysis/SemanticAnalyser.hpp"

namespace EmojicodeCompiler {

void ASTIf::analyse(SemanticAnalyser *analyser) {
    for (size_t i = 0; i < conditions_.size(); i++) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        analyser->expectType(Type::boolean(), &conditions_[i]);
        blocks_[i].analyse(analyser);
        analyser->scoper().popScope(analyser->app());
        analyser->pathAnalyser().beginBranch();
    }

    if (hasElse()) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        blocks_.back().analyse(analyser);
        analyser->scoper().popScope(analyser->app());
        analyser->pathAnalyser().endBranch();

        analyser->pathAnalyser().endMutualExclusiveBranches();
    }
    else {
        analyser->pathAnalyser().endUncertainBranches();
    }

    certainlyReturned_ = analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::Returned);
}

void ASTRepeatWhile::analyse(SemanticAnalyser *analyser) {
    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    analyser->expectType(Type::boolean(), &condition_);
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->app());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endUncertainBranches();
}

void ASTErrorHandler::analyse(SemanticAnalyser *analyser) {
    Type type = analyser->expect(TypeExpectation(false, false), &value_);

    if (type.type() != TypeType::Error) {
        throw CompilerError(position(), "🥑 can only be used with 🚨.");
    }

    analyser->scoper().pushScope();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();

    valueIsBoxed_ = type.storageType() == StorageType::Box;
    valueType_ = type.genericArguments()[1];
    if (valueIsBoxed_) {
        valueType_.forceBox();
    }
    analyser->scoper().currentScope().declareVariable(valueVarName_, valueType_, true, position()).initialize();
    valueBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->app());
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    analyser->scoper().currentScope().declareVariable(errorVarName_, type.genericArguments()[0], true,
                                                      position()).initialize();

    errorBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->app());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endMutualExclusiveBranches();
    analyser->scoper().popScope(analyser->app());
}

void ASTForIn::analyse(SemanticAnalyser *analyser) {
    analyser->scoper().pushScope();

    Type iteratee = analyser->expect(TypeExpectation(true, true, false), &iteratee_);

    elementType_ = Type::noReturn();
    if (!analyser->typeIsEnumerable(iteratee, &elementType_)) {
        auto iterateeString = iteratee.toString(analyser->typeContext());
        throw CompilerError(position(), iterateeString, " does not conform to s🔂.");
    }

    iteratee_->setExpressionType(Type(PR_ENUMERATEABLE, false));

    analyser->pathAnalyser().beginBranch();
    auto &elVar = analyser->scoper().currentScope().declareVariable(varName_, elementType_, true, position());
    elVar.initialize();
    elementVar_ = elVar.id();
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->app());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endUncertainBranches();
}

}  // namespace EmojicodeCompiler
