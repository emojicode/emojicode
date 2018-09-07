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
    popScope(function_->ast());
}

void MFFunctionAnalyser::analyseFunctionCall(ASTArguments *node, ASTExpr *callee, Function *function) {
    if (function->memoryFlowTypeForThis() == MFType::Unknown) {
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
    bool release = block->stopsAtReturn();
    auto bound = block->returnedCertainly() ? block->scopeStats().allVariablesCount : block->scopeStats().variables;
    auto from = block->returnedCertainly() ? 0 : block->scopeStats().from;
    for (size_t i = 0; i < bound; i++) {
        VariableID variableId = i + from;
        auto &var = scope_.getVariable(variableId);
        if (var.isParam) {
            function_->setParameterMFType(var.param, var.mfType);
        }
        else if (release && !var.isReturned && var.type.isManaged()) {
            block->appendNodeBeforeReturn(std::make_unique<ASTRelease>(false, variableId, var.type,
                                                                       block->position()));
        }

        if (var.mfType == MFType::Borrowing) {
            for (auto init : var.inits) {
                init->allocateOnStack();
            }
        }
    }
}

void MFFunctionAnalyser::recordVariableGet(size_t id, MFType type) {
    if (type == MFType::Escaping) {
        scope_.getVariable(id).mfType = type;
    }
}

void MFFunctionAnalyser::retain(std::shared_ptr<ASTExpr> *expr) {
    (*expr)->analyseMemoryFlow(this, MFType::Escaping);
    (*expr)->unsetIsTemporary();
}

void MFFunctionAnalyser::recordReturn(std::shared_ptr<ASTExpr> *expr) {
    (*expr)->unsetIsTemporary();
    if (auto getVar = dynamic_cast<ASTGetVariable *>(expr->get())) {
        if (!getVar->inInstanceScope()) {
            auto &var = scope_.getVariable(getVar->id());
            if (var.isParam) return;
            var.isReturned = true;
        }
    }
    if (auto getVar = dynamic_cast<ASTThis *>(expr->get())) {
        return;
    }
    (*expr)->analyseMemoryFlow(this, MFType::Escaping);
}

void MFFunctionAnalyser::recordThis(MFType type) {
    if (type == MFType::Escaping) {
        thisEscapes_ = true;
    }
}

void MFFunctionAnalyser::recordVariableSet(size_t id, ASTExpr *expr, bool init, Type type) {
    auto &var = scope_.getVariable(id);
    var.type = std::move(type);
    if (auto heapAllocates = dynamic_cast<MFHeapAllocates *>(expr)) {
        var.inits.emplace_back(heapAllocates);
    }
}

}  // namespace EmojicodeCompiler
