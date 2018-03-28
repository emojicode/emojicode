//
//  LLVMTypeHelper.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 06/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Functions/Function.hpp"
#include "Compiler.hpp"
#include "Functions/Initializer.hpp"
#include "Generation/ReificationContext.hpp"
#include "LLVMTypeHelper.hpp"
#include "Package/Package.hpp"
#include "Mangler.hpp"
#include "CodeGenerator.hpp"
#include "Scoping/CapturingSemanticScoper.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/Class.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <AST/ASTClosure.hpp>

namespace EmojicodeCompiler {

LLVMTypeHelper::LLVMTypeHelper(llvm::LLVMContext &context, CodeGenerator *codeGenerator)
        : context_(context), codeGenerator_(codeGenerator) {
    protocolsTable_ = llvm::StructType::create(std::vector<llvm::Type *> {
            llvm::Type::getInt1Ty(context_),
            llvm::Type::getInt8PtrTy(context_)->getPointerTo(),
    }, "protocolConformance");
    classMetaType_ = llvm::StructType::create(std::vector<llvm::Type *> {
            llvm::Type::getInt64Ty(context_), llvm::Type::getInt8PtrTy(context_)->getPointerTo()
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

    auto compiler = codeGenerator_->package()->compiler();
    types_.emplace(Type::noReturn(), llvm::Type::getVoidTy(context_));
    types_.emplace(Type::someobject(), llvm::Type::getInt8PtrTy(context_));
    types_.emplace(Type(compiler->sInteger), llvm::Type::getInt64Ty(context_));
    types_.emplace(Type(compiler->sSymbol), llvm::Type::getInt32Ty(context_));
    types_.emplace(Type(compiler->sReal), llvm::Type::getDoubleTy(context_));
    types_.emplace(Type(compiler->sBoolean), llvm::Type::getInt1Ty(context_));
    types_.emplace(Type(compiler->sMemory), llvm::Type::getInt8PtrTy(context_));
    types_.emplace(Type(compiler->sByte), llvm::Type::getInt8Ty(context_));
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
    else if (hasThisArgument(function->functionType()) &&
            !(function->functionType() == FunctionType::ObjectInitializer && function->owningType().klass()->foreign())) {
        args.emplace_back(llvmTypeFor(function->typeContext().calleeType()));
    }
    std::transform(function->parameters().begin(), function->parameters().end(), std::back_inserter(args), [this](auto &arg) {
        return llvmTypeFor(arg.type);
    });
    llvm::Type *returnType;
    if (function->functionType() == FunctionType::ObjectInitializer) {
        auto init = dynamic_cast<Initializer *>(function);
        returnType = llvmTypeFor(init->constructedType(init->typeContext().calleeType()));
    }
    else {
        returnType = llvmTypeFor(function->returnType());
    }
    return llvm::FunctionType::get(returnType, args, false);
}

llvm::Type* LLVMTypeHelper::box() const {
    return box_;
}

llvm::Type* LLVMTypeHelper::valueTypeMetaPtr() const {
    return valueTypeMetaType_->getPointerTo();
}

bool LLVMTypeHelper::isDereferenceable(const Type &type) const {
    return ((type.type() == TypeType::Class || type.type() == TypeType::Someobject) &&
            type.storageType() != StorageType::Box) || type.isReference();
}

bool LLVMTypeHelper::isRemote(const Type &type) {
    return codeGenerator_->querySize(llvmTypeFor(type)) > 32;
}

llvm::Type* LLVMTypeHelper::llvmTypeFor(const Type &type) {
    if (reifiContext_ != nullptr && type.type() == TypeType::LocalGenericVariable &&
            reifiContext_->providesActualTypeFor(type.genericVariableIndex())) {
        return llvmTypeFor(reifiContext_->actualType(type.genericVariableIndex()));
    }

    auto llvmType = typeForOrdinaryType(type);
    assert(llvmType != nullptr);
    return type.isReference() ? llvmType->getPointerTo() : llvmType;
}

llvm::Type* LLVMTypeHelper::typeForOrdinaryType(const Type &type) {
    switch (type.storageType()) {
        case StorageType::Box:
            return box_;
        case StorageType::SimpleOptional: {
            std::vector<llvm::Type *> types{ llvm::Type::getInt1Ty(context_), llvmTypeFor(type.optionalType()) };
            return llvm::StructType::get(context_, types);
        }
        case StorageType::SimpleError: {
            std::vector<llvm::Type *> types{ llvmTypeFor(type.errorEnum()), llvmTypeFor(type.errorType()) };
            return llvm::StructType::get(context_, types);
        }
        case StorageType::Simple:
            return getSimpleType(type);
    }
}

llvm::Type* LLVMTypeHelper::getSimpleType(const Type &type) {
    if (type.type() == TypeType::Callable) {
        return callable_;
    }
    if (type.type() == TypeType::TypeAsValue) {
        if (type.typeOfTypeValue().type() == TypeType::Class) {
            return classMetaType_->getPointerTo();
        }
        return llvm::StructType::get(context_);
    }
    if (type.type() == TypeType::Enum) {
        return llvm::Type::getInt64Ty(context_);
    }
    return getComposedType(type);
}

llvm::Type *LLVMTypeHelper::getComposedType(const Type &type) {
    llvm::Type *llvmType = nullptr;
    auto it = types_.find(type);
    if (it != types_.end()) {
        llvmType = it->second;
    }
    else if (type.type() == TypeType::ValueType || type.type() == TypeType::Class) {
        llvmType = createLlvmTypeForTypeDefinition(type);
    }
    else {
        throw std::logic_error("No llvm type could be established.");
    }

    if (type.type() == TypeType::Class) {
        llvmType = llvmType->getPointerTo();
    }
    return llvmType;
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

}  // namespace EmojicodeCompiler
