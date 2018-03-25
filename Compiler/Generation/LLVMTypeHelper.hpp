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

namespace llvm {
class Type;
class StructType;
}  // namespace llvm

namespace EmojicodeCompiler {

class Function;
struct VariableCapture;
class Compiler;
class ReificationContext;
struct Capture;

/// This class is responsible for providing llvm::Type instances for Emojicode Type instances.
///
/// Per package one LLVMTypeHelper must be used. It is created by the CodeGenerator. Do not instantiate a LLVMTypeHelper
/// otherwise.
class LLVMTypeHelper {
public:
    explicit LLVMTypeHelper(llvm::LLVMContext &context, Compiler *compiler);

    /// @returns An LLVM type corresponding to the provided Type.
    /// @throws std::logic_error if no type can be established. This will normally not happen.
    llvm::Type* llvmTypeFor(Type type);
    /// @returns The LLVM type representing boxes.
    llvm::Type* box() const;
    /// @returns An LLVM function type (a signature) matching the provided Function.
    /// @throws std::logic_error if no type can be established. This will normally not happen.
    llvm::FunctionType* functionTypeFor(Function *function);

    /// @returns True if it is guaranteed that the provided type is represnted as a pointer at run-time that can always
    /// be dereferenced.
    bool isDereferenceable(const Type &type) const;

    llvm::Type* valueTypeMetaPtr() const;
    llvm::StructType* valueTypeMeta() const { return valueTypeMetaType_; }
    llvm::StructType* classMeta() const { return classMetaType_; }
    llvm::StructType* protocolConformance() const { return protocolsTable_; }

    llvm::StructType *llvmTypeForCapture(const Capture &capture, llvm::Type *thisType);

    void setReificationContext(ReificationContext *context) { reifiContext_ = context; };

private:
    llvm::StructType *classMetaType_;
    llvm::StructType *valueTypeMetaType_;
    llvm::StructType *box_;
    llvm::StructType *protocolsTable_;
    llvm::StructType *callable_;

    llvm::Type* getSimpleType(const Type &type);

    llvm::LLVMContext &context_;

    std::map<Type, llvm::Type*> types_;
    ReificationContext *reifiContext_ = nullptr;

    llvm::Type *typeForOrdinaryType(Type type);

    llvm::Type* createLlvmTypeForTypeDefinition(const Type &type);
    llvm::Type *getComposedType(const Type &type);
};

}  // namespace EmojicodeCompiler

#endif /* LLVMTypeHelper_hpp */
