//
//  ASTControlFlow.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Types/Class.hpp"
#include "AST/ASTNode.hpp"
#include "ASTControlFlow.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Types/Protocol.hpp"

namespace EmojicodeCompiler {

void ASTIf::analyse(FunctionAnalyser *analyser) {
    for (size_t i = 0; i < conditions_.size(); i++) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        analyser->expectType(analyser->boolean(), &conditions_[i]);
        blocks_[i].analyse(analyser);
        analyser->scoper().popScope(analyser->compiler());
        analyser->pathAnalyser().endBranch();
    }

    if (hasElse()) {
        analyser->pathAnalyser().beginBranch();
        analyser->scoper().pushScope();
        blocks_.back().analyse(analyser);
        analyser->scoper().popScope(analyser->compiler());
        analyser->pathAnalyser().endBranch();

        analyser->pathAnalyser().endMutualExclusiveBranches();
    }
    else {
        analyser->pathAnalyser().endUncertainBranches();
    }
}

void ASTRepeatWhile::analyse(FunctionAnalyser *analyser) {
    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    analyser->expectType(analyser->boolean(), &condition_);
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endUncertainBranches();
}

void ASTErrorHandler::analyse(FunctionAnalyser *analyser) {
    Type type = analyser->expect(TypeExpectation(false, false), &value_);

    if (type.type() != TypeType::Error) {
        throw CompilerError(position(), "🥑 can only be used with 🚨.");
    }

    analyser->scoper().pushScope();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();

    valueIsBoxed_ = type.storageType() == StorageType::Box;
    valueType_ = type.errorType();
    if (valueIsBoxed_) {
        valueType_.forceBox();
    }
    auto &var = analyser->scoper().currentScope().declareVariable(valueVarName_, valueType_, true, position());
    var.initialize();
    valueVar_ = var.id();
    valueBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().beginBranch();
    analyser->scoper().pushScope();
    auto &errVar = analyser->scoper().currentScope().declareVariable(errorVarName_, type.errorEnum(), true, position());
    errVar.initialize();
    errorVar_ = errVar.id();
    errorBlock_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();

    analyser->pathAnalyser().endMutualExclusiveBranches();
    analyser->scoper().popScope(analyser->compiler());
}

void ASTForIn::analyse(FunctionAnalyser *analyser) {
    analyser->scoper().pushScope();

    auto iterateeType = Type(analyser->compiler()->sEnumerable);
    iterateeType.setReference();
    Type iteratee = analyser->expectType(iterateeType, &iteratee_);

    elementType_ = Type::noReturn();
    if (!typeIsEnumerable(analyser, &elementType_, iteratee)) {
        auto iterateeString = iteratee.toString(analyser->typeContext());
        throw CompilerError(position(), iterateeString, " does not conform to s🔂.");
    }

    iteratee_->setExpressionType(Type(analyser->compiler()->sEnumerable));

    analyser->pathAnalyser().beginBranch();
    auto &elVar = analyser->scoper().currentScope().declareVariable(varName_, elementType_, true, position());
    elVar.initialize();
    elementVar_ = elVar.id();
    block_.analyse(analyser);
    analyser->scoper().popScope(analyser->compiler());
    analyser->pathAnalyser().endBranch();
    analyser->pathAnalyser().endUncertainBranches();
}

bool EmojicodeCompiler::ASTForIn::typeIsEnumerable(FunctionAnalyser *analyser, Type *elementType, const Type &type) {
    if (type.type() == TypeType::Class) {
        for (Class *a = type.klass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.protocol() == analyser->compiler()->sEnumerable) {
                    auto itemType = Type(0, analyser->compiler()->sEnumerable, true);
                    *elementType = itemType.resolveOn(TypeContext(protocol.resolveOn(TypeContext(type))));
                    return true;
                }
            }
        }
    }
    else if (type.canHaveProtocol()) {
        for (auto &protocol : type.typeDefinition()->protocols()) {
            if (protocol.protocol() == analyser->compiler()->sEnumerable) {
                auto itemType = Type(0, analyser->compiler()->sEnumerable, true);
                *elementType = itemType.resolveOn(TypeContext(protocol.resolveOn(TypeContext(type))));
                return true;
            }
        }
    }
    else if (type.type() == TypeType::Protocol && type.protocol() == analyser->compiler()->sEnumerable) {
        *elementType = Type(0, type.protocol(), true).resolveOn(TypeContext(type));
        return true;
    }
    return false;
}


}  // namespace EmojicodeCompiler
