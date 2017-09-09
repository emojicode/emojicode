//
//  ASTVariables.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Application.hpp"
#include "../Scoping/VariableNotFoundError.hpp"
#include "ASTInitialization.hpp"

namespace EmojicodeCompiler {

Type ASTGetVariable::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto var = analyser->scoper().getVariable(name_, position());
    copyVariableAstInfo(var, analyser);
    return var.variable.type();
}

void ASTInitableCreator::setVtDestination(SemanticAnalyser *analyser, VariableID varId, bool inInstanceScope,
                                          bool declare, const Type &type) {
    if (auto init = std::dynamic_pointer_cast<ASTInitialization>(expr_)) {
        if (init->initType() == ASTInitialization::InitType::ValueType) {
            noAction_ = true;
            declare_ = declare;
            type_ = type;
        }
    }
}

void ASTVariableDeclaration::analyse(SemanticAnalyser *analyser) {
    auto &var = analyser->scoper().currentScope().declareVariable(varName_, type_, false, position());
    id_ = var.id();
}

void ASTVariableAssignmentDecl::analyse(SemanticAnalyser *analyser) {
    try {
        auto rvar = analyser->scoper().getVariable(varName_, position());
        if (rvar.inInstanceScope && !analyser->function()->mutating() &&
            !isFullyInitializedCheckRequired(analyser->function()->functionType())) {
            analyser->app()->error(CompilerError(position(),
                                                 "Can’t mutate instance variable as method is not marked with 🖍."));
        }

        copyVariableAstInfo(rvar, analyser);

        rvar.variable.initialize();
        rvar.variable.mutate(position());

        analyser->expectType(rvar.variable.type(), &expr_);
        setVtDestination(analyser, rvar.variable.id(), rvar.inInstanceScope, false, rvar.variable.type());
    }
    catch (VariableNotFoundError &vne) {
        // Not declared, declaring as local variable
        declare_ = true;
        Type t = analyser->expect(TypeExpectation(false, true), &expr_);
        auto &var = analyser->scoper().currentScope().declareVariable(varName_, t, false, position());
        var.initialize();
        copyVariableAstInfo(ResolvedVariable(var, false), analyser);
        setVtDestination(analyser, var.id(), false, true, t);
    }
}

void ASTInstanceVariableAssignment::analyse(SemanticAnalyser *analyser) {
    auto &var = analyser->scoper().instanceScope()->getLocalVariable(varName_);
    var.initialize();
    var.mutate(position());
    copyVariableAstInfo(ResolvedVariable(var, true), analyser);
    analyser->expectType(var.type(), &expr_);
}

void ASTFrozenDeclaration::analyse(SemanticAnalyser *analyser) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    auto &var = analyser->scoper().currentScope().declareVariable(varName_, t, true, position());
    var.initialize();
    id_ = var.id();
    setVtDestination(analyser, var.id(), false, true, t);
}

}  // namespace EmojicodeCompiler
