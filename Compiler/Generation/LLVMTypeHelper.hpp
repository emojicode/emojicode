//
//  LLVMTypeHelper.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 06/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef LLVMTypeHelper_hpp
#define LLVMTypeHelper_hpp

#include "Types/Type.hpp"
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <memory>
#include <functional>

namespace llvm {
class Type;
class StructType;
class ArrayType;
class PointerType;
class FunctionType;
}  // namespace llvm

namespace EmojicodeCompiler {

class Function;
struct VariableCapture;
class ReificationContext;
struct Capture;
class CodeGenerator;
struct TypeDefinitionReification;
template <typename Entity>
struct Reification;

/// This class is responsible for providing llvm::Type instances for Emojicode Type instances.
///
/// Per package one LLVMTypeHelper must be used. It is created by the CodeGenerator. Do not instantiate a LLVMTypeHelper
/// otherwise.
class LLVMTypeHelper {
public:
    explicit LLVMTypeHelper(llvm::LLVMContext &context, CodeGenerator *codeGenerator);

    /// @returns An LLVM type corresponding to the provided Type.
    /// @throws std::logic_error if no type can be established. This will normally not happen.
    llvm::Type* llvmTypeFor(const Type &type);
    /// @returns The LLVM type representing boxes.
    llvm::Type* box() const;
    /// @returns An LLVM function type (a signature) matching the provided Function.
    /// @throws std::logic_error if no type can be established. This will normally not happen.
    llvm::FunctionType* functionTypeFor(Function *function);

    /// @returns True if it is guaranteed that the provided type is represnted as a pointer at run-time that can always
    /// be dereferenced.
    bool isDereferenceable(const Type &type) const;

    /// @returns True if this type cannot be directly stored in a box and memory must be allocated on the heap.
    bool isRemote(const Type &type);

    /// A pointer to a value of this type is stored in the first field of a box to identify its content.
    llvm::StructType* boxInfo() const { return boxInfoType_; }
    /// The class info stores the dispatch table as well as a pointer to the class info of the super class if this class
    /// has a superclass.
    llvm::StructType* classInfo() const { return classInfoType_; }
    llvm::StructType* protocolConformance() const { return protocolsTable_; }
    llvm::PointerType* someobject() const { return someobjectPtr_; }
    llvm::FunctionType* boxRetainRelease() const { return boxRetainRelease_; }
    llvm::FunctionType* captureDeinit() const { return captureDeinit_; }
    llvm::StructType* protocolConformanceEntry() const { return protocolConformanceEntry_; }

    llvm::StructType* llvmTypeForCapture(const Capture &capture, llvm::Type *thisType);
    llvm::ArrayType* multiprotocolConformance(const Type &type);

    llvm::StructType* callable() const { return callable_; }

    /// Wraps the provided type into an anonymous struct where the first element is a control block pointer and the
    /// second the type.
    ///
    /// This can be used to allocate objects with FunctionCodeGenerator::alloc and the like if they do not normally
    /// have a control block pointer.
    llvm::StructType* managable(llvm::Type *type) const;

    void withReificationContext(ReificationContext context, std::function<void()> function);

    const Reification<TypeDefinitionReification>* ownerReification() const;

    ~LLVMTypeHelper();

private:
    llvm::StructType *classInfoType_;
    llvm::StructType *boxInfoType_;
    llvm::StructType *box_;
    llvm::StructType *protocolsTable_;
    llvm::StructType *callable_;
    llvm::PointerType *someobjectPtr_;
    llvm::FunctionType *boxRetainRelease_;
    llvm::FunctionType *captureDeinit_;
    llvm::StructType *protocolConformanceEntry_;

    llvm::Type* getSimpleType(const Type &type);

    llvm::LLVMContext &context_;
    CodeGenerator *codeGenerator_;

    std::unique_ptr<ReificationContext> reifiContext_;

    llvm::Type *typeForOrdinaryType(const Type &type);

    llvm::Type* llvmTypeForTypeDefinition(const Type &type);
};

}  // namespace EmojicodeCompiler

#endif /* LLVMTypeHelper_hpp */
