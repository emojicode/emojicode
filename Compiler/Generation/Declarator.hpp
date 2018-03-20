//
// Created by Theo Weidmann on 03.02.18.
//

#ifndef EMOJICODE_DECLARATOR_HPP
#define EMOJICODE_DECLARATOR_HPP

#include "Types/Class.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

class LLVMTypeHelper;

/// The declarator is responsible for declaring functions etc. in a given LLVM module.
class Declarator {
public:
    Declarator(llvm::LLVMContext &context, llvm::Module &module, LLVMTypeHelper &typeHelper);

    llvm::Function* runTimeNew() const { return runTimeNew_; }
    llvm::Function* panic() const { return panic_; }
    llvm::GlobalVariable* classValueTypeMeta() { return classValueTypeMeta_; }

    void declareImportedPackageSymbols(Package *package);

    /// Declares an LLVM function for each reification of the provided function.
    void declareLlvmFunction(Function *function);
private:
    void declareImportedClassMeta(Class *klass);

    llvm::LLVMContext &context_;
    llvm::Module &module_;
    LLVMTypeHelper &typeHelper_;

    llvm::Function *runTimeNew_;
    llvm::Function *panic_;
    llvm::GlobalVariable *classValueTypeMeta_;

    llvm::Function* declareRunTimeFunction(const char *name, llvm::Type *returnType, llvm::ArrayRef<llvm::Type *> args);
    void declareRunTime();
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_DECLARATOR_HPP
