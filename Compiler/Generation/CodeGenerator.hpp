//
//  CodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CodeGenerator_hpp
#define CodeGenerator_hpp

#include "Declarator.hpp"
#include "LLVMTypeHelper.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "StringPool.hpp"
#include "OptimizationManager.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <string>
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
    explicit CodeGenerator(Package *package, bool optimize);
    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() const { return module_.get(); }

    LLVMTypeHelper& typeHelper() { return typeHelper_; }
    StringPool& stringPool() { return pool_; }
    Declarator& declarator() { return declarator_; }

    /// Returns the package for which this code generator was created.
    Package* package() const { return package_; }

    /// Generates an object file for the package.
    /// @param outPath The path at which the object file will be placed.
    void generate(const std::string &outPath, bool printIr);

    /// Queries the DataLayout of the module for the size of this type in bytes.
    /// @note Use this method only when absolutely necessary. Prefer FunctionCodeGenerator::sizeOf in all function
    /// code.
    uint64_t querySize(llvm::Type *type) const;

    llvm::Value* optionalValue();
    llvm::Value* optionalNoValue();

    llvm::Constant * valueTypeMetaFor(const Type &type);
private:
    Package *const package_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;

    LLVMTypeHelper typeHelper_;
    StringPool pool_ = StringPool(this);
    Declarator declarator_;
    ProtocolsTableGenerator protocolsTableGenerator_;
    OptimizationManager optimizationManager_;

    llvm::TargetMachine *targetMachine_ = nullptr;

    void emit(const std::string &outPath, bool printIr);
    void generateFunctions();

    void generateFunction(Function *function);
    void createClassInfo(Class *klass);

    void createProtocolFunctionTypes(Protocol *protocol);

    void prepareModule();
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
