//
// Created by Theo Weidmann on 25.03.18.
//

#include "OptimizationManager.hpp"
#include "ReferenceCountingPasses.hpp"
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

namespace EmojicodeCompiler {

OptimizationManager::OptimizationManager(llvm::Module *module, bool optimize, RunTimeHelper *runTime)
        : optimize_(optimize), functionPassManager_(std::make_unique<llvm::legacy::FunctionPassManager>(module)),
            passManager_(std::make_unique<llvm::legacy::PassManager>()) {
                initialize(runTime);
            }

void OptimizationManager::initialize(RunTimeHelper *runTime) {
    if (optimize_) {
        llvm::PassManagerBuilder builder;
        builder.OptLevel = 3;
        builder.SizeLevel = 0;
        builder.Inliner = llvm::createFunctionInliningPass();
        builder.MergeFunctions = true;

        passManager_->add(new LocalReferenceCountingPass(runTime));

        builder.populateFunctionPassManager(*functionPassManager_);
        builder.populateModulePassManager(*passManager_);

        functionPassManager_->add(llvm::createInductiveRangeCheckEliminationPass());
        functionPassManager_->add(llvm::createLICMPass());
        functionPassManager_->doInitialization();

        passManager_->add(new ConstantReferenceCountingPass(runTime));
        passManager_->add(new RedundantReferenceCountingPass(runTime));
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
