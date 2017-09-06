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
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "../Types/Type.hpp"
#include <map>
#include <string>

namespace EmojicodeCompiler {

class Application;

/// Manages the generation of IR for one package. Each package is compiled to one LLVM module.
class CodeGenerator {
public:
    CodeGenerator(Package *package);
    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() { return module_.get(); }

    /// Returns the package for which this code generator was created.
    Package* package() const { return package_; }

    void generate(const std::string &outPath);

    llvm::Value* optionalValue();
    llvm::Value* optionalNoValue();

    llvm::Type* llvmTypeForType(Type type);
    llvm::Type* box() const;
    llvm::Type* valueTypeMetaTypePtr() const;

    llvm::Function* runTimeNew() const { return runTimeNew_; }
private:
    llvm::Type* createLlvmTypeForTypeDefinition(const Type &type);

    Package *package_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    std::map<Type, llvm::Type*> types_;
    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

    llvm::Function *runTimeNew_;
    llvm::StructType *classMetaType_;
    llvm::StructType *valueTypeMetaType_;
    llvm::StructType *box_;

    void setUpPassManager();
    void emit(const std::string &outPath);
    void declareRunTime();
    void declareLlvmFunction(Function *function);
    void generateFunction(Function *function);
    void createClassInfo(Class *klass);
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
