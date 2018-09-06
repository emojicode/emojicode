//
//  ASTVariables.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Generation/FunctionCodeGenerator.hpp"
#include "ASTBinaryOperator.hpp"
#include "ASTInitialization.hpp"
#include "ASTVariables.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Scoping/VariableNotFoundError.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

void AccessesAnyVariable::setVariableAccess(const ResolvedVariable &var, FunctionAnalyser *analyser) {
    id_ = var.variable.id();
    inInstanceScope_ = var.inInstanceScope;
    variableType_ = var.variable.type();
    if (inInstanceScope_) {
        analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    }
}

Type ASTGetVariable::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    auto var = analyser->scoper().getVariable(name(), position());
    setVariableAccess(var, analyser);
    var.variable.uninitalizedError(position());
    return var.variable.type();
}

void ASTGetVariable::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFType type) {
    if (!inInstanceScope()) {
        analyser->recordVariableGet(id(), type);
    }
}

void ASTVariableDeclaration::analyse(FunctionAnalyser *analyser) {
    auto &type = type_->analyseType(analyser->typeContext());
    auto &var = analyser->scoper().currentScope().declareVariable(varName_, type, false, position());
    if (type.type() == TypeType::Optional) {
        var.initialize();
    }
    id_ = var.id();
}

void ASTVariableAssignment::analyse(FunctionAnalyser *analyser) {
    auto rvar = analyser->scoper().getVariable(name(), position());

    if (rvar.inInstanceScope && !analyser->function()->mutating() &&
        !isFullyInitializedCheckRequired(analyser->function()->functionType())) {
        analyser->compiler()->error(CompilerError(position(),
                                                  "Can’t mutate instance variable as method is not marked with 🖍."));
    }

    setVariableAccess(rvar, analyser);
    analyser->expectType(rvar.variable.type(), &expr_);

    wasInitialized_ = rvar.variable.isInitialized();

    rvar.variable.initialize();
    rvar.variable.mutate(position());
}

void ASTVariableAssignment::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    analyser->retain(&expr_);
    if (!inInstanceScope()) {
        analyser->recordVariableSet(id(), expr_.get(), false, variableType());
    }
}

void ASTVariableDeclareAndAssign::analyse(FunctionAnalyser *analyser) {
    Type t = analyser->expect(TypeExpectation(false, true), &expr_).inexacted();
    auto &var = analyser->scoper().currentScope().declareVariable(name(), t, false, position());
    var.initialize();
    setVariableAccess(ResolvedVariable(var, false), analyser);
}

void ASTVariableDeclareAndAssign::analyseMemoryFlow(EmojicodeCompiler::MFFunctionAnalyser *analyser) {
    analyser->retain(&expr_);
    analyser->recordVariableSet(id(), expr_.get(), true, variableType());
}

void ASTInstanceVariableInitialization::analyse(FunctionAnalyser *analyser) {
    auto &var = analyser->scoper().instanceScope()->getLocalVariable(name());
    var.initialize();
    var.mutate(position());
    setVariableAccess(ResolvedVariable(var, true), analyser);
    analyser->expectType(var.type(), &expr_);
}

void ASTConstantVariable::analyse(FunctionAnalyser *analyser) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    auto &var = analyser->scoper().currentScope().declareVariable(name(), t, true, position());
    var.initialize();
    setVariableAccess(ResolvedVariable(var, false), analyser);
}

void ASTConstantVariable::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    analyser->retain(&expr_);
    analyser->recordVariableSet(id(), expr_.get(), false, expr_->expressionType());
}

ASTOperatorAssignment::ASTOperatorAssignment(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                                             const SourcePosition &p, OperatorType opType) :
ASTVariableAssignment(name,
                      std::make_shared<ASTBinaryOperator>(opType, std::make_shared<ASTGetVariable>(name, p), e, p),
                      p) {}

}  // namespace EmojicodeCompiler
