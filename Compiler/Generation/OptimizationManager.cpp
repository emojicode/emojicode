//
// Created by Theo Weidmann on 25.03.18.
//

#include "OptimizationManager.hpp"
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace EmojicodeCompiler {

OptimizationManager::OptimizationManager(llvm::Module *module, bool optimize)
        : optimize_(optimize), functionPassManager_(std::make_unique<llvm::legacy::FunctionPassManager>(module)),
            passManager_(std::make_unique<llvm::legacy::PassManager>()) {}

void OptimizationManager::initialize() {
    if (optimize_) {
        llvm::PassManagerBuilder builder;
        builder.OptLevel = 3;
        builder.SizeLevel = 0;
        builder.Inliner = llvm::createFunctionInliningPass();

        builder.populateFunctionPassManager(*functionPassManager_);
        builder.populateModulePassManager(*passManager_);

        functionPassManager_->add(llvm::createInductiveRangeCheckEliminationPass());
        functionPassManager_->add(llvm::createLICMPass());
        functionPassManager_->doInitialization();
    }
}

void OptimizationManager::optimize(llvm::Function *function) {
    if (optimize_) {
        functionPassManager_->run(*function);
    }
}

void OptimizationManager::optimize(llvm::Module *module) {
    if (optimize_) {
        passManager_->run(*module);
    }
}

}  // namespace EmojicodeCompiler
