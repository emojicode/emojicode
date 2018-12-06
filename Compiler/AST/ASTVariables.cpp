//
//  ASTVariables.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTVariables.hpp"
#include "ASTBinaryOperator.hpp"
#include "ASTInitialization.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Scoping/VariableNotFoundError.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

void AccessesAnyVariable::setVariableAccess(const ResolvedVariable &var, ExpressionAnalyser *analyser) {
    id_ = var.variable.id();
    inInstanceScope_ = var.inInstanceScope;
    variableType_ = var.variable.type();
    if (inInstanceScope_) {
        analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    }
}

Type ASTGetVariable::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto var = analyser->scoper().getVariable(name(), position());
    setVariableAccess(var, analyser);
    var.variable.uninitalizedError(position());
    auto type = var.variable.type();
    if (var.inInstanceScope && analyser->typeContext().calleeType().type() == TypeType::ValueType &&
        !analyser->typeContext().calleeType().isMutable()) {
        type.setMutable(false);
    }
    return type;
}

void ASTGetVariable::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    if (type.isReturn()) {
        returned_ = true;
    }
    if (!inInstanceScope()) {
        analyser->recordVariableGet(id(), type);
    }
}

Type ASTIsOnlyReference::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto rvar = analyser->scoper().getVariable(name(), position());
    if (rvar.variable.type().type() != TypeType::Someobject && rvar.variable.type().type() != TypeType::Class) {
        analyser->compiler()->error(CompilerError(position(), "🏮 can only be used with objects."));
    }
    setVariableAccess(rvar, analyser);
    return analyser->boolean();
}

void ASTIsOnlyReference::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
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
    analyser->take(expr_.get());
    if (!inInstanceScope()) {
        analyser->recordVariableSet(id(), expr_.get(), variableType());
    }
    else {
        expr_->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);
    }
}

void ASTVariableDeclareAndAssign::analyse(FunctionAnalyser *analyser) {
    Type t = analyser->expect(TypeExpectation(false, true), &expr_).inexacted();
    auto &var = analyser->scoper().currentScope().declareVariable(name(), t, false, position());
    var.initialize();
    setVariableAccess(ResolvedVariable(var, false), analyser);
}

void ASTVariableDeclareAndAssign::analyseMemoryFlow(EmojicodeCompiler::MFFunctionAnalyser *analyser) {
    analyser->take(expr_.get());
    analyser->recordVariableSet(id(), expr_.get(), variableType());
}

void ASTInstanceVariableInitialization::analyse(FunctionAnalyser *analyser) {
    auto &var = analyser->scoper().instanceScope()->getLocalVariable(name());
    var.initialize();
    var.mutate(position());
    setVariableAccess(ResolvedVariable(var, true), analyser);
    if (analyseExpr_) {
        analyser->expectType(var.type(), &expr_);
    }
}

void ASTConstantVariable::analyse(FunctionAnalyser *analyser) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    auto &var = analyser->scoper().currentScope().declareVariable(name(), t, true, position());
    var.initialize();
    setVariableAccess(ResolvedVariable(var, false), analyser);
}

void ASTConstantVariable::analyseMemoryFlow(MFFunctionAnalyser *analyser) {
    analyser->take(expr_.get());
    analyser->recordVariableSet(id(), expr_.get(), expr_->expressionType());
}

ASTOperatorAssignment::ASTOperatorAssignment(std::u32string name, const std::shared_ptr<ASTExpr> &e,
                                             const SourcePosition &p, OperatorType opType) :
ASTVariableAssignment(name,
                      std::make_shared<ASTBinaryOperator>(opType, std::make_shared<ASTGetVariable>(name, p), e, p),
                      p) {}

}  // namespace EmojicodeCompiler
