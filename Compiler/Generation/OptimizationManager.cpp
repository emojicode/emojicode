//
// Created by Theo Weidmann on 25.03.18.
//

#include "OptimizationManager.hpp"
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

namespace EmojicodeCompiler {

OptimizationManager::OptimizationManager(llvm::Module *module, bool optimize)
        : optimize_(optimize), functionPassManager_(llvm::make_unique<llvm::legacy::FunctionPassManager>(module)) {
    if (optimize_) {
        functionPassManager_->add(llvm::createPromoteMemoryToRegisterPass());
        functionPassManager_->add(llvm::createInstructionCombiningPass());
        functionPassManager_->add(llvm::createReassociatePass());
        functionPassManager_->add(llvm::createGVNPass());
        functionPassManager_->add(llvm::createCFGSimplificationPass());
        functionPassManager_->add(llvm::createLICMPass());
        functionPassManager_->add(llvm::createIndVarSimplifyPass());
        functionPassManager_->add(llvm::createPartiallyInlineLibCallsPass());

        functionPassManager_->add(llvm::createConstantPropagationPass());
        functionPassManager_->add(llvm::createDeadCodeEliminationPass());

        functionPassManager_->doInitialization();
    }
}

void OptimizationManager::optimize(llvm::Function *function) {
    if (optimize_) {
        functionPassManager_->run(*function);
    }
    // llvm::outs() << *function;
}

}  // namespace EmojicodeCompiler