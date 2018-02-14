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
#include "StringPool.hpp"
#include "Declarator.hpp"
#include "ProtocolsTableGenerator.hpp"
#include <memory>
#include <vector>

namespace EmojicodeCompiler {

class Compiler;
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
    StringPool& stringPool() { return pool_; }
    Declarator& declarator() { return declarator_; }

    /// Returns the package for which this code generator was created.
    Package* package() const { return package_; }

    /// Generates an object file for the package.
    /// @param outPath The path at which the object file will be placed.
    void generate(const std::string &outPath);

    llvm::Value* optionalValue();
    llvm::Value* optionalNoValue();

    llvm::GlobalVariable* valueTypeMetaFor(const Type &type);
private:
    Package *const package_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;

    std::unique_ptr<llvm::legacy::FunctionPassManager> passManager_;

    LLVMTypeHelper typeHelper_;
    StringPool pool_ = StringPool(this);
    Declarator declarator_;
    ProtocolsTableGenerator protocolsTableGenerator_;

    void setUpPassManager();
    void emit(const std::string &outPath);
    void generateFunctions();

    void generateFunction(Function *function);
    void createClassInfo(Class *klass);

    void createProtocolFunctionTypes(Protocol *protocol);
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
