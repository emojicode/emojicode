//
//  LLVMTypeHelper.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 06/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef LLVMTypeHelper_hpp
#define LLVMTypeHelper_hpp

#include <llvm/IR/LLVMContext.h>
#include "../Types/Type.hpp"
#include <map>

namespace llvm {
class Type;
}  // namespace llvm

namespace EmojicodeCompiler {

class Function;

/// This class is repsonsible for providing llvm::Type instances for Emojicode Type instances.
class LLVMTypeHelper {
public:
    explicit LLVMTypeHelper(llvm::LLVMContext &context);

    llvm::Type* llvmTypeFor(Type type);
    llvm::Type* box() const;
    llvm::Type* valueTypeMetaPtr() const;
    llvm::StructType* valueTypeMeta() const { return valueTypeMetaType_; }
    llvm::StructType* classMeta() const { return classMetaType_; }
    llvm::StructType* protocolsTable() const { return protocolsTable_; }

    llvm::Type* createLlvmTypeForTypeDefinition(const Type &type);
    llvm::FunctionType* functionTypeFor(Function *function);
private:
    llvm::StructType *classMetaType_;
    llvm::StructType *valueTypeMetaType_;
    llvm::StructType *box_;
    llvm::StructType *protocolsTable_;
    llvm::StructType *callable_;

    llvm::Type* getSimpleType(const Type &type);

    llvm::LLVMContext &context_;

    std::map<Type, llvm::Type*> types_;
};

}  // namespace EmojicodeCompiler

#endif /* LLVMTypeHelper_hpp */
