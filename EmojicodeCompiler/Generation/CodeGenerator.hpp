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
#include <vector>

namespace EmojicodeCompiler {

class Application;
class Package;
class Class;
class Function;
class TypeDefinition;

/// Manages the generation of IR for one package. Each package is compiled to one LLVM module.
class CodeGenerator {
public:
    explicit CodeGenerator(Package *package);
    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() { return module_.get(); }

    LLVMTypeHelper& typeHelper() { return typeHelper_; }

    /// Returns the package for which this code generator was created.
    Package* package() const { return package_; }

    /// Generates an object file for the package.
    /// @param outPath The path at which the object file will be placed.
    void generate(const std::string &outPath, const std::vector<std::unique_ptr<Package>> &dependencies);

    llvm::Value* optionalValue();
    llvm::Value* optionalNoValue();
    llvm::GlobalVariable* classValueTypeMeta() { return classValueTypeMeta_; }

    llvm::GlobalVariable* valueTypeMetaFor(const Type &type);

    llvm::Function* runTimeNew() const { return runTimeNew_; }
    llvm::Function* errNoValue() const { return errNoValue_; }
private:
    Package *package_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;

    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

    llvm::Function *runTimeNew_;
    llvm::Function *errNoValue_;
    llvm::GlobalVariable *classValueTypeMeta_;
    LLVMTypeHelper typeHelper_ = LLVMTypeHelper(context());

    struct ProtocolVirtualTables {
        ProtocolVirtualTables(std::vector<llvm::Constant *> tables, size_t max, size_t min)
        : tables(std::move(tables)), min(min), max(max) {}
        std::vector<llvm::Constant *> tables;
        size_t min;
        size_t max;
    };

    void setUpPassManager();
    void emit(const std::string &outPath);
    void generateFunctions();
    void declarePackageSymbols();
    void declareRunTime();
    void declareImportedPackageSymbols(Package *package);
    void declareLlvmFunction(Function *function);
    void generateFunction(Function *function);
    void createClassInfo(Class *klass);
    void createProtocolsTable(TypeDefinition *typeDef);
    void createProtocolFunctionTypes(Protocol *protocol);
    llvm::Function* declareRunTimeFunction(const char *name, llvm::Type *returnType, llvm::ArrayRef<llvm::Type *> args);
    ProtocolVirtualTables createProtocolVirtualTables(TypeDefinition *typeDef);
    llvm::GlobalVariable* createProtocolVirtualTable(TypeDefinition *typeDef, Protocol *protocol);
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
