//
//  ASTVariables.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Generation/FunctionCodeGenerator.hpp"
#include "ASTVariables.hpp"
#include "ASTInitialization.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "Scoping/VariableNotFoundError.hpp"

namespace EmojicodeCompiler {

void AccessesAnyVariable::setVariableAccess(const ResolvedVariable &var, SemanticAnalyser *analyser) {
    id_ = var.variable.id();
    inInstanceScope_ = var.inInstanceScope;
    if (inInstanceScope_) {
        analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    }
}

Type ASTGetVariable::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto var = analyser->scoper().getVariable(name(), position());
    setVariableAccess(var, analyser);
    return var.variable.type();
}

void ASTVariableDeclaration::analyse(SemanticAnalyser *analyser) {
    auto &var = analyser->scoper().currentScope().declareVariable(varName_, type_, false, position());
    id_ = var.id();
}

void ASTVariableAssignmentDecl::analyse(SemanticAnalyser *analyser) {
    try {
        auto rvar = analyser->scoper().getVariable(name(), position());
        if (rvar.inInstanceScope && !analyser->function()->mutating() &&
            !isFullyInitializedCheckRequired(analyser->function()->functionType())) {
            analyser->compiler()->error(CompilerError(position(),
                                                      "Can’t mutate instance variable as method is not marked with 🖍."));
        }

        setVariableAccess(rvar, analyser);
        rvar.variable.initialize();
        rvar.variable.mutate(position());

        analyser->expectType(rvar.variable.type(), &expr_);
    }
    catch (VariableNotFoundError &vne) {
        // Not declared, declaring as local variable
        setDeclares();
        Type t = analyser->expect(TypeExpectation(false, true), &expr_);
        auto &var = analyser->scoper().currentScope().declareVariable(name(), t, false, position());
        var.initialize();
        setVariableAccess(ResolvedVariable(var, false), analyser);
    }
}

void ASTInstanceVariableInitialization::analyse(SemanticAnalyser *analyser) {
    auto &var = analyser->scoper().instanceScope()->getLocalVariable(name());
    var.initialize();
    var.mutate(position());
    setVariableAccess(ResolvedVariable(var, true), analyser);
    analyser->expectType(var.type(), &expr_);
}

void ASTFrozenDeclaration::analyse(SemanticAnalyser *analyser) {
    setDeclares();
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    auto &var = analyser->scoper().currentScope().declareVariable(name(), t, true, position());
    var.initialize();
    setVariableAccess(ResolvedVariable(var, false), analyser);
}

}  // namespace EmojicodeCompiler
