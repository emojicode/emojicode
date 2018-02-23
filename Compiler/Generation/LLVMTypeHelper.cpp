//
//  LLVMTypeHelper.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 06/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "LLVMTypeHelper.hpp"
#include "Mangler.hpp"
#include "Scoping/CapturingSemanticScoper.hpp"
#include "Types/TypeDefinition.hpp"
#include "Compiler.hpp"
#include "Generation/ReificationContext.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <AST/ASTClosure.hpp>

namespace EmojicodeCompiler {

LLVMTypeHelper::LLVMTypeHelper(llvm::LLVMContext &context, Compiler *compiler) : context_(context) {
    protocolsTable_ = llvm::StructType::create(std::vector<llvm::Type *> {
        llvm::Type::getInt8PtrTy(context_)->getPointerTo()->getPointerTo(), llvm::Type::getInt16Ty(context_),
        llvm::Type::getInt16Ty(context_)
    }, "protocolsTable");
    classMetaType_ = llvm::StructType::create(std::vector<llvm::Type *> {
        llvm::Type::getInt64Ty(context_), llvm::Type::getInt8PtrTy(context_)->getPointerTo(), protocolsTable_
    }, "classMeta");
    valueTypeMetaType_ = llvm::StructType::create(std::vector<llvm::Type *> {
        protocolsTable_
    }, "valueTypeMeta");
    box_ = llvm::StructType::create(std::vector<llvm::Type *> {
        valueTypeMetaType_->getPointerTo(), llvm::ArrayType::get(llvm::Type::getInt8Ty(context_), 32),
    }, "box");
    callable_ = llvm::StructType::create(std::vector<llvm::Type *> {
        llvm::Type::getInt8PtrTy(context_), llvm::Type::getInt8PtrTy(context_)
    }, "callable");

    types_.emplace(Type::noReturn(), llvm::Type::getVoidTy(context_));
    types_.emplace(Type::someobject(), llvm::Type::getInt8PtrTy(context_));
    types_.emplace(Type(compiler->sInteger, false), llvm::Type::getInt64Ty(context_));
    types_.emplace(Type(compiler->sSymbol, false), llvm::Type::getInt32Ty(context_));
    types_.emplace(Type(compiler->sReal, false), llvm::Type::getDoubleTy(context_));
    types_.emplace(Type(compiler->sBoolean, false), llvm::Type::getInt1Ty(context_));
    types_.emplace(Type(compiler->sMemory, false), llvm::Type::getInt8PtrTy(context_));
}

llvm::StructType* LLVMTypeHelper::llvmTypeForCapture(const Capture &capture, llvm::Type *thisType) {
    std::vector<llvm::Type *> types;
    if (capture.captureSelf) {
        types.emplace_back(thisType);
    }
    std::transform(capture.captures.begin(), capture.captures.end(), std::back_inserter(types), [this](auto &capture) {
        return llvmTypeFor(capture.type);
    });
    return llvm::StructType::get(context_, types);
}

llvm::FunctionType* LLVMTypeHelper::functionTypeFor(Function *function) {
    std::vector<llvm::Type *> args;
    if (function->functionType() == FunctionType::Closure) {
        args.emplace_back(llvm::Type::getInt8PtrTy(context_));
    }
    else if (hasThisArgument(function->functionType())) {
        args.emplace_back(llvmTypeFor(function->typeContext().calleeType()));
    }
    std::transform(function->arguments.begin(), function->arguments.end(), std::back_inserter(args), [this](auto &arg) {
        return llvmTypeFor(arg.type);
    });
    llvm::Type *returnType;
    if (function->functionType() == FunctionType::ObjectInitializer) {
        auto init = dynamic_cast<Initializer *>(function);
        returnType = llvmTypeFor(init->constructedType(init->typeContext().calleeType()));
    }
    else {
        returnType = llvmTypeFor(function->returnType);
    }
    return llvm::FunctionType::get(returnType, args, false);
}

llvm::Type* LLVMTypeHelper::createLlvmTypeForTypeDefinition(const Type &type) {
    std::vector<llvm::Type *> types;

    if (type.type() == TypeType::Class) {
        types.emplace_back(classMetaType_->getPointerTo());
    }

    for (auto &ivar : type.typeDefinition()->instanceVariables()) {
        types.emplace_back(llvmTypeFor(ivar.type));
    }

    auto llvmType = llvm::StructType::create(context_, types, mangleTypeName(type));
    types_.emplace(type, llvmType);
    return llvmType;
}

llvm::Type* LLVMTypeHelper::box() const {
    return box_;
}

llvm::Type* LLVMTypeHelper::valueTypeMetaPtr() const {
    return valueTypeMetaType_->getPointerTo();
}

llvm::Type* LLVMTypeHelper::llvmTypeFor(Type type) {
    if (reifiContext_ != nullptr && type.type() == TypeType::LocalGenericVariable &&
            reifiContext_->providesActualTypeFor(type.genericVariableIndex())) {
        return llvmTypeFor(reifiContext_->actualType(type.genericVariableIndex()));
    }

    llvm::Type *llvmType = nullptr;

    if (type.meta()) {
        return classMetaType_->getPointerTo();
    }

    // Error is always a simple optional, so we have to catch it before switching below
    if (type.type() == TypeType::Error) {
        std::vector<llvm::Type *> types{ llvm::Type::getInt1Ty(context_), llvmTypeFor(type.genericArguments()[1]) };
        llvmType = llvm::StructType::get(context_, types);
    }

    switch (type.storageType()) {
        case StorageType::Box:
            llvmType = box_;
            break;
        case StorageType::SimpleOptional: {
            type.setOptional(false);
            std::vector<llvm::Type *> types{ llvm::Type::getInt1Ty(context_), llvmTypeFor(type) };
            llvmType = llvm::StructType::get(context_, types);
            break;
        }
        case StorageType::Simple:
            llvmType = getSimpleType(type);
            break;
    }
    assert(llvmType != nullptr);
    return type.isReference() ? llvmType->getPointerTo() : llvmType;
}

llvm::Type* LLVMTypeHelper::getSimpleType(const Type &type) {
    if (type.type() == TypeType::Callable) {
        return callable_;
    }

    llvm::Type *llvmType = nullptr;
    auto it = types_.find(type);
    if (it != types_.end()) {
        llvmType = it->second;
    }
    else if (type.type() == TypeType::ValueType || type.type() == TypeType::Class) {
        llvmType = createLlvmTypeForTypeDefinition(type);
    }
    else if (type.type() == TypeType::Enum) {
        llvmType = llvm::Type::getInt64Ty(context_);
    }
    else {
        throw std::logic_error("No llvm type could be established.");
    }
    if (type.type() == TypeType::Class) {
        llvmType = llvmType->getPointerTo();
    }
    return llvmType;
}

}  // namespace EmojicodeCompiler
