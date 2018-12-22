//
//  CodeGenerator.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CodeGenerator_hpp
#define CodeGenerator_hpp

#include "LLVMTypeHelper.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <map>

namespace llvm {
class TargetMachine;
}  // namespace llvm

namespace EmojicodeCompiler {

class Compiler;
class Package;
class Class;
class Function;
class TypeDefinition;
class ProtocolsTableGenerator;
class StringPool;
class Declarator;
class OptimizationManager;

/// Manages the generation of IR for a package. Each package is compiled to one LLVM module.
class CodeGenerator {
public:
    CodeGenerator(Compiler *compiler);

    /// Generates an object file for the package.
    /// @param outPath The path at which the object file will be placed.
    /// @param optimize Whether optimizations should be run.
    void generate(Package *package, const std::string &outPath, bool printIr, bool optimize);

    /// The LLVM module that represents the package.
    llvm::Module* module() const { return module_.get(); }

    LLVMTypeHelper& typeHelper() { return typeHelper_; }
    StringPool& stringPool() { return *pool_; }
    Declarator& declarator() { return *declarator_; }
    ProtocolsTableGenerator& protocolsTG() { return *protocolsTableGenerator_; }
    llvm::LLVMContext& context() { return context_; }

    Compiler* compiler() const;

    /// Queries the DataLayout of the module for the size of this type in bytes.
    /// @note Use this method only when absolutely necessary. Prefer FunctionCodeGenerator::sizeOf in all function
    /// code.
    uint64_t querySize(llvm::Type *type) const;

    /// Determines a box info value that is used to identify values of the provided type in a box.
    /// @returns An LLVM value representing the box info that must be stored in the box info field.
    llvm::Constant* boxInfoFor(const Type &type);

    llvm::Constant* protocolIdentifierFor(const Type &type);

    ~CodeGenerator();

private:
    Compiler *const compiler_;
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;

    LLVMTypeHelper typeHelper_;
    std::unique_ptr<StringPool> pool_;
    std::unique_ptr<Declarator> declarator_;
    std::unique_ptr<ProtocolsTableGenerator> protocolsTableGenerator_;
    std::unique_ptr<OptimizationManager> optimizationManager_;

    void declareAndCreate(Package *package, bool imported);

    llvm::TargetMachine *targetMachine_ = nullptr;

    std::pair<llvm::Function*, llvm::Function*> classObjectRetainRelease_ = { nullptr, nullptr };

    std::pair<llvm::Function*, llvm::Function*> buildBoxRetainRelease(const Type &type);
    void buildClassObjectBoxInfo();
    void buildCallableBoxInfo();

    void emitModule(const std::string &outPath, bool printIr);
    void generateFunctions(Package *package, bool imported);

    void generateFunction(Function *function, const Reification<TypeDefinitionReification> *reification);
    void createClassInfo(Class *klass);

    void generateTypeDefinition(TypeDefinition *typeDef, bool isClass);

    void createProtocolFunctionTypes(Protocol *protocol);

    void prepareModule(Package *package, bool optimize);

    std::map<Type, llvm::Constant*> protocolIds_;
};

}  // namespace EmojicodeCompiler

#endif /* CodeGenerator_hpp */
