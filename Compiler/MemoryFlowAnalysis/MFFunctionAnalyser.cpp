//
//  MFFunctionAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 01/06/2018.
//  Copyright © 2018 Theo Weidmann. All rights reserved.
//

#include "AST/ASTExpr.hpp"
#include "AST/ASTVariables.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTLiterals.hpp"
#include "Functions/Function.hpp"
#include "MFFunctionAnalyser.hpp"
#include "MFHeapAllocates.hpp"
#include "Scoping/SemanticScopeStats.hpp"

namespace EmojicodeCompiler {

MFFunctionAnalyser::MFFunctionAnalyser(Function *function) : scope_(function->variableCount()), function_(function) {}

void MFFunctionAnalyser::analyse() {
    if (function_->isExternal()) {
        return;
    }

    function_->setMemoryFlowTypeForThis(MFType::Escaping);

    for (size_t i = 0; i < function_->parameters().size(); i++) {
        auto &var = scope_.getVariable(i);
        var.isParam = true;
        var.param = i;
    }

    function_->ast()->analyseMemoryFlow(this);
    function_->setMemoryFlowTypeForThis(thisEscapes_ ? MFType::Escaping : MFType::Borrowing);
    popScope(function_->ast()->scopeStats());
}

void MFFunctionAnalyser::analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function) {
    if (function->memoryFlowTypeForThis() == MFType::Unknown) {
        MFFunctionAnalyser(function).analyse();
    }
    if (callee != nullptr) {
        callee->analyseMemoryFlow(this, function->memoryFlowTypeForThis());
    }
    for (size_t i = 0; i < node->parameters().size(); i++) {
        node->parameters()[i]->analyseMemoryFlow(this, function->parameters()[i].memoryFlowType);
    }
}

void MFFunctionAnalyser::popScope(const SemanticScopeStats &stats) {
    for (size_t i = 0; i < stats.variables; i++) {
        auto &var = scope_.getVariable(i + stats.from);
        if (var.isParam) {
            function_->setParameterMFType(var.param, var.type);
        }
        else {
            update(var);
        }
    }
}

void MFFunctionAnalyser::update(MFLocalVariable &variable) {
    assert(!variable.isParam);
    if (variable.type != MFType::Borrowing) return;
    for (auto init : variable.inits) {
        init->allocateOnStack();
    }
}

void MFFunctionAnalyser::recordVariableGet(size_t id, MFType type) {
    if (type == MFType::Escaping) {
        scope_.getVariable(id).type = type;
    }
}

void MFFunctionAnalyser::take(std::shared_ptr<ASTExpr> *expr) {
    (*expr)->analyseMemoryFlow(this, MFType::Escaping);
}

void MFFunctionAnalyser::recordReturn(std::shared_ptr<ASTExpr> *expr) {
    if (auto getVar = dynamic_cast<ASTGetVariable *>(expr->get())) {
        if (!getVar->inInstanceScope() && scope_.getVariable(getVar->id()).isParam) {
            return;
        }
    }
    if (auto getVar = dynamic_cast<ASTThis *>(expr->get())) {
        return;
    }
    take(expr);
}

void MFFunctionAnalyser::recordThis(MFType type) {
    if (type == MFType::Escaping) {
        thisEscapes_ = true;
    }
}

void MFFunctionAnalyser::recordVariableSet(size_t id, ASTExpr *expr, bool init) {
    auto &var = scope_.getVariable(id);
    if (auto heapAllocates = dynamic_cast<MFHeapAllocates *>(expr)) {
        var.inits.emplace_back(heapAllocates);
    }
}

}  // namespace EmojicodeCompiler
