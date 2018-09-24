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
#include "AST/ASTMemory.hpp"
#include "Functions/Function.hpp"
#include "MFFunctionAnalyser.hpp"
#include "MFHeapAllocates.hpp"
#include "Scoping/SemanticScopeStats.hpp"

namespace EmojicodeCompiler {

MFFunctionAnalyser::MFFunctionAnalyser(Function *function) : scope_(function->variableCount()), function_(function) {}

void MFFunctionAnalyser::analyse() {
    if (function_->isExternal() || function_->memoryFlowTypeForThis() != MFFlowCategory::Unknown) {
        return;
    }

    function_->setMemoryFlowTypeForThis(MFFlowCategory::Escaping);

    for (size_t i = 0; i < function_->parameters().size(); i++) {
        auto &var = scope_.getVariable(i);
        var.isParam = true;
        var.param = i;
    }

    function_->ast()->analyseMemoryFlow(this);
    function_->setMemoryFlowTypeForThis(thisEscapes_ ? MFFlowCategory::Escaping : MFFlowCategory::Borrowing);
    popScope(function_->ast());
}

void MFFunctionAnalyser::analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function) {
    if (function->memoryFlowTypeForThis() == MFFlowCategory::Unknown) {
        MFFunctionAnalyser(function).analyse();
    }
    if (callee != nullptr) {
        callee->analyseMemoryFlow(this, function->memoryFlowTypeForThis());
    }
    for (size_t i = 0; i < node->args().size(); i++) {
        node->args()[i]->analyseMemoryFlow(this, function->parameters()[i].memoryFlowType);
    }
}

void MFFunctionAnalyser::popScope(ASTBlock *block) {
    releaseVariables(block);

    for (size_t i = 0; i < block->scopeStats().variables; i++) {
        auto &var = scope_.getVariable(i + block->scopeStats().from);
        if (var.isParam) {
            function_->setParameterMFType(var.param, var.flowCategory);
        }
        else if (var.flowCategory == MFFlowCategory::Borrowing) {
            for (auto init : var.inits) {
                init->allocateOnStack();
            }
        }
    }
}

void MFFunctionAnalyser::releaseVariables(ASTBlock *block) {
    // If this block does not return certainly we can simply release the variables declared.
    // If it does, however, we must make certain that it is the one that directly contains the return statement, and
    // if and only if it is, we must release all variables defined in the function.
    if (!block->returnedCertainly() || block->stopsAtReturn()) {
        auto bound = block->returnedCertainly() ? block->scopeStats().allVariablesCount : block->scopeStats().variables;
        auto from = block->returnedCertainly() ? 0 : block->scopeStats().from;
        for (size_t i = 0; i < bound; i++) {
            VariableID variableId = i + from;
            auto &var = scope_.getVariable(variableId);
            if (!var.isParam && !var.isReturned && var.type.isManaged()) {
                block->appendNodeBeforeReturn(std::make_unique<ASTRelease>(false, variableId, var.type,
                                                                           block->position()));
            }
        }
    }
}

void MFFunctionAnalyser::recordVariableGet(size_t id, MFFlowCategory category) {
    if (category == MFFlowCategory::Escaping) {
        scope_.getVariable(id).flowCategory = category;
    }
}

void MFFunctionAnalyser::take(ASTExpr *expr) {
    expr->unsetIsTemporary();
}

void MFFunctionAnalyser::recordReturn(ASTExpr *expr) {
    take(expr);
    if (auto getVar = dynamic_cast<ASTGetVariable *>(expr)) {
        if (!getVar->inInstanceScope()) {
            auto &var = scope_.getVariable(getVar->id());
            if (var.isParam) return;
            var.isReturned = true;
        }
    }
    if (auto getVar = dynamic_cast<ASTThis *>(expr)) {
        return;
    }
    expr->analyseMemoryFlow(this, MFFlowCategory::Escaping);
}

void MFFunctionAnalyser::recordThis(MFFlowCategory category) {
    if (category == MFFlowCategory::Escaping) {
        thisEscapes_ = true;
    }
}

void MFFunctionAnalyser::recordVariableSet(size_t id, ASTExpr *expr, Type type) {
    auto &var = scope_.getVariable(id);
    var.type = std::move(type);
    if (expr != nullptr) {
        expr->analyseMemoryFlow(this, MFFlowCategory::Escaping);
        if (auto heapAllocates = dynamic_cast<MFHeapAllocates *>(expr)) {
            var.inits.emplace_back(heapAllocates);
        }
    }
}

}  // namespace EmojicodeCompiler
