//
// Created by Theo Weidmann on 03.02.18.
//

#ifndef EMOJICODE_DECLARATOR_HPP
#define EMOJICODE_DECLARATOR_HPP

#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "RunTimeTypeInfoFlags.hpp"

namespace EmojicodeCompiler {

class CodeGenerator;
class Class;
class Function;
class Package;
class Type;
class ReificationContext;
class TypeDefinition;

/// This class provides the Emojicode run-time library interface, and helps declare and manage run-time infomration
/// such as run-time type information.
class RunTimeHelper {
public:
    RunTimeHelper(CodeGenerator *generator);

    /// @pre This function depends on that the associated CodeGenerator was initialized.
    void declareRunTime();

    /// The allocator function that is called to allocate all heap memory. (ejcAlloc)
    llvm::Function* alloc() const { return alloc_; }
    /// The panic method, which is called if the program panics due to e.g. unwrapping an empty optional. (ejcPanic)
    llvm::Function* panic() const { return panic_; }
    /// The function that is called to determine if one class inherits from another. (ejcInheritsFrom)
    llvm::Function* inheritsFrom() const { return inheritsFrom_; }
    /// The function called to retain any value. (ejcRetain)
    llvm::Function* retain() const { return retain_; }
    /// The function that must be used to release objects.
    /// @note Use releaseMemory() to release memory areas that do not represent class instances!
    llvm::Function* release() const { return release_; }
    /// The function that is to be used to release memory area that do not represent objects.
    /// (ejcReleaseMemory)
    /// @see release
    llvm::Function* releaseMemory() const { return releaseMemory_; }
    /// The function that is used to release object that whose initialization has been aborted.
    /// @see release
    llvm::Function* releaseWithoutDeinit() const { return releaseWithoutDeinit_; }
    /// The function that is to be used to release closure captures.
    /// (ejcReleaseCapture)
    /// @see release
    llvm::Function* releaseCapture() const { return releaseCapture_; }
    /// Used to find a protocol conformance in an array of ProtocolConformanceEntries. (ejcFindProtocolConformance)
    llvm::Function* findProtocolConformance() const { return findProtocolConformance_; }

    llvm::Function* checkGenericArgs() const { return checkGenericArgs_; }
    llvm::Function* typeDescriptionLength() const { return typeDescriptionLength_; }
    llvm::Function* indexTypeDescription() const { return indexTypeDescription_; }

    llvm::Function* isOnlyReference() const { return isOnlyReference_; }

    llvm::GlobalVariable* ignoreBlockPtr() const { return ignoreBlock_; }

    /// Declares the box info with the provided name. This is a global variable without initializer.
    llvm::GlobalVariable* declareBoxInfo(const std::string &name);

    llvm::GlobalVariable* boxInfoForObjects() { return boxInfoClassObjects_; }
    llvm::GlobalVariable* boxInfoForCallables() { return boxInfoCallables_; }

    std::pair<llvm::Function*, llvm::Function*> classObjectRetainRelease() const { return classObjectRetainRelease_; }

    llvm::Constant* createRtti(TypeDefinition *generic, RunTimeTypeInfoFlags::Flags flag);

    llvm::GlobalVariable *somethingRtti() const { return somethingRTTI_; }
    llvm::GlobalVariable *someobjectRtti() const { return someobjectRTTI_; }

private:
    CodeGenerator *generator_;

    llvm::Function *alloc_ = nullptr;
    llvm::Function *panic_ = nullptr;

    llvm::Function *inheritsFrom_ = nullptr;
    llvm::Function *findProtocolConformance_ = nullptr;
    llvm::Function *checkGenericArgs_ = nullptr;
    llvm::Function *typeDescriptionLength_ = nullptr;
    llvm::Function *indexTypeDescription_ = nullptr;

    llvm::GlobalVariable *boxInfoClassObjects_ = nullptr;
    llvm::GlobalVariable *boxInfoCallables_ = nullptr;
    llvm::GlobalVariable *ignoreBlock_ = nullptr;

    llvm::Function *retain_ = nullptr;
    llvm::Function *release_ = nullptr;
    llvm::Function *releaseMemory_ = nullptr;
    llvm::Function *releaseCapture_ = nullptr;
    llvm::Function *releaseWithoutDeinit_ = nullptr;
    llvm::Function *isOnlyReference_ = nullptr;

    llvm::GlobalVariable *somethingRTTI_ = nullptr;
    llvm::GlobalVariable *someobjectRTTI_ = nullptr;

    std::pair<llvm::Function*, llvm::Function*> classObjectRetainRelease_ = { nullptr, nullptr };

    llvm::Function* declareRunTimeFunction(const char *name, llvm::Type *returnType, llvm::ArrayRef<llvm::Type *> args);
    llvm::Function* declareMemoryRunTimeFunction(const char *name);
    llvm::GlobalVariable* createAbstractRtti(const char *name);

    std::pair<llvm::Function*, llvm::Function*> buildRetainRelease(const Type &prototype, const char *retainName,
                                                                   const char *releaseName,
                                                                   llvm::GlobalVariable *boxInfo);
};

}  // namespace EmojicodeCompiler

#endif //EMOJICODE_DECLARATOR_HPP
