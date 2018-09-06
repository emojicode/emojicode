//
// Created by Theo Weidmann on 03.02.18.
//

#ifndef EMOJICODE_DECLARATOR_HPP
#define EMOJICODE_DECLARATOR_HPP

#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>

namespace EmojicodeCompiler {

class CodeGenerator;
class Class;
class Function;
class Package;

/// The declarator is responsible for declaring functions etc. in an LLVM module.
class Declarator {
public:
    Declarator(CodeGenerator *generator);

    /// The allocator function that is called to allocate all heap memory. (ejcAlloc)
    llvm::Function* alloc() const { return alloc_; }
    /// The panic method, which is called if the program panics due to e.g. unwrapping an empty optional. (ejcPanic)
    llvm::Function* panic() const { return panic_; }
    /// The function that is called to determine if one class inherits from another. (ejcInheritsFrom)
    llvm::Function* inheritsFrom() const { return inheritsFrom_; }
    /// The function called to retain any value allocated with ::alloc. (ejcRetain)
    llvm::Function* retain() const { return retain_; }
    /// The function that must be used to release objects allocated with ::alloc.
    /// @note Use releaseMemory() to release memory areas that do not represent class instances!
    llvm::Function* release() const { return release_; }
    /// The function that is to be used to release memory area allocated with ::alloc that do not represent objects.
    /// (ejcReleaseMemory)
    /// @see release
    llvm::Function* releaseMemory() const { return releaseMemory_; }
    /// Used to find a protocol conformance in an array of ProtocolConformanceEntries. (ejcFindProtocolConformance)
    llvm::Function* findProtocolConformance() const { return findProtocolConformance_; }

    /// Declares all symbols that are provided by an imported package.
    /// @param package The package whose symbols shall be declared.
    void declareImportedPackageSymbols(Package *package);
    /// Declares an LLVM function for each reification of the provided function.
    void declareLlvmFunction(Function *function) const;

    /// Declares the box info with the provided name. This is a global variable without initializer.
    llvm::GlobalVariable* declareBoxInfo(const std::string &name);

    llvm::GlobalVariable* boxInfoForObjects() { return boxInfoClassObjects_; }

private:
    void declareImportedClassInfo(Class *klass);

    CodeGenerator *generator_;

    llvm::Function *alloc_ = nullptr;
    llvm::Function *panic_ = nullptr;

    llvm::Function *inheritsFrom_ = nullptr;
    llvm::Function *findProtocolConformance_ = nullptr;

    llvm::GlobalVariable *boxInfoClassObjects_;

    llvm::Function *retain_ = nullptr;
    llvm::Function *release_ = nullptr;
    llvm::Function *releaseMemory_ = nullptr;

    llvm::Function* declareRunTimeFunction(const char *name, llvm::Type *returnType, llvm::ArrayRef<llvm::Type *> args);
    void declareRunTime();
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_DECLARATOR_HPP
