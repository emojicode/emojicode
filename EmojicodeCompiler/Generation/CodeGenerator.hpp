  //
//  CodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CodeGenerator_hpp
#define CodeGenerator_hpp

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <string>
#include "LLVMTypeHelper.hpp"
#include <memory>

namespace EmojicodeCompiler {

class Application;
class Package;
class Class;
class Function;

/// Manages the generation of IR for one package. Each package is compiled to one LLVM module.
class CodeGenerator {
public:
    explicit CodeGenerator(Package *package);
    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() { return module_.get(); }

    LLVMTypeHelper& typeHelper() { return typeHelper_; }

    /// Returns the package for which this code generator was created.
    Package* package() const { return package_; }

    void generate(const std::string &outPath);

    llvm::Value* optionalValue();
    llvm::Value* optionalNoValue();

    llvm::GlobalVariable* valueTypeMetaFor(const Type &type);

    llvm::Function* runTimeNew() const { return runTimeNew_; }
private:
    Package *package_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;

    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

    llvm::Function *runTimeNew_;
    llvm::GlobalVariable *classValueTypeMeta_;
    LLVMTypeHelper typeHelper_ = LLVMTypeHelper(context());

    void setUpPassManager();
    void emit(const std::string &outPath);
    void generateFunctions();
    void declarePackageSymbols();
    void declareRunTime();
    void declareLlvmFunction(Function *function);
    void generateFunction(Function *function);
    void createClassInfo(Class *klass);
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
