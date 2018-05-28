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

class OptimizationManager {
public:
    explicit OptimizationManager(llvm::Module *module, bool optimize);
    void optimize(llvm::Function *function);
    void initialize();
private:
    bool optimize_;
    std::unique_ptr<llvm::legacy::FunctionPassManager> functionPassManager_;
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_OPTIMIZATIONMANAGER_HPP
