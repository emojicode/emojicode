//
// Created by Theo Weidmann on 25.03.18.
//

#ifndef EMOJICODE_OPTIMIZATIONMANAGER_HPP
#define EMOJICODE_OPTIMIZATIONMANAGER_HPP

#include <llvm/IR/LegacyPassManager.h>
#include <memory>

namespace llvm {
class Function;
}  // namespace llvm

namespace EmojicodeCompiler {

class RunTimeHelper;

class OptimizationManager {
public:
    explicit OptimizationManager(llvm::Module *module, bool optimize, RunTimeHelper *runTime);
    void optimize(llvm::Function *function);
    void optimize(llvm::Module *module);
    void initialize(RunTimeHelper *runTime);
private:
    bool optimize_;
    std::unique_ptr<llvm::legacy::FunctionPassManager> functionPassManager_;
    std::unique_ptr<llvm::legacy::PassManager> passManager_;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_OPTIMIZATIONMANAGER_HPP
