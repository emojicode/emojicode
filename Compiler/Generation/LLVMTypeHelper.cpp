//
//  LLVMTypeHelper.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 06/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Functions/Function.hpp"
#include "CodeGenerator.hpp"
#include "Compiler.hpp"
#include "Functions/Initializer.hpp"
#include "Generation/ReificationContext.hpp"
#include "LLVMTypeHelper.hpp"
#include "Mangler.hpp"
#include "Package/Package.hpp"
#include "Scoping/CapturingSemanticScoper.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeContext.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <AST/ASTClosure.hpp>

namespace EmojicodeCompiler {

/// The number of bytes a box provides for storing value type data.
const unsigned kBoxSize = 32;

LLVMTypeHelper::LLVMTypeHelper(llvm::LLVMContext &context, CodeGenerator *codeGenerator)
        : context_(context), codeGenerator_(codeGenerator), mdBuilder_(context) {

    runTimeTypeInfo_ = llvm::StructType::create({
        llvm::Type::getInt16Ty(context_),  // generic parameter count
        llvm::Type::getInt16Ty(context_),  // generic parameter offset (for subclasses)
        llvm::Type::getInt8Ty(context_), // flag (see RunTimeTypeInfoFlags)
    }, "runTimeTypeInfo");

    typeDescription_ = llvm::StructType::create(context_, "typeDescription");
    typeDescription_->setBody({
        // Pointer to the generic type info of the described type.
        // The address itself is used to determine whether to types are equal!
        runTimeTypeInfo_->getPointerTo(),
        llvm::Type::getInt1Ty(context_)  // optional
    });

    boxInfoType_ = llvm::StructType::create(context_, "boxInfo");
    box_ = llvm::StructType::create(context_, "box");

    boxRetainRelease_ = llvm::FunctionType::get(llvm::Type::getVoidTy(context_), box()->getPointerTo(), false);

    protocolsTable_ = llvm::StructType::create({
        llvm::Type::getInt1Ty(context_),  // whether the boxed value itself is the callee (i.e. value type) or not
        llvm::Type::getInt8PtrTy(context_)->getPointerTo(),
        boxInfoType_->getPointerTo(),
        boxRetainRelease_->getPointerTo(), boxRetainRelease_->getPointerTo()
    }, "protocolConformance");
    protocolConformanceEntry_ = llvm::StructType::create({
        llvm::Type::getInt1PtrTy(context_), protocolsTable_->getPointerTo() }, "protocolConformanceEntry");

    boxInfoType_->setBody({
        runTimeTypeInfo_,  // must be first so that we can cast back and forth between boxInfo and runTimeTypeInfo
        boxRetainRelease_->getPointerTo(),
        boxRetainRelease_->getPointerTo(),
        protocolConformanceEntry_->getPointerTo()
    });

    box_->setBody({
        boxInfoType_->getPointerTo(), llvm::ArrayType::get(llvm::Type::getInt8Ty(context_), kBoxSize),
    });

    classInfoType_ = llvm::StructType::create(context_, "classInfo");
    classInfoType_->setBody({
        runTimeTypeInfo_,  // must be first so that we can cast back and forth between classInfo and runTimeTypeInfo
        llvm::Type::getInt8PtrTy(context_)->getPointerTo(),
        protocolConformanceEntry_->getPointerTo(),
        classInfoType_->getPointerTo()
    });

    callable_ = llvm::StructType::create({
        llvm::Type::getInt8PtrTy(context_),  // function pointer
        llvm::Type::getInt8PtrTy(context_)  // capture pointer
    }, "callable");

    someobjectPtr_ = llvm::StructType::create({
        llvm::Type::getInt8PtrTy(context_),  // control block
        classInfoType_->getPointerTo()
    }, "someobject")->getPointerTo();

    captureDeinit_ = llvm::FunctionType::get(llvm::Type::getVoidTy(context_),
                                             llvm::Type::getInt8PtrTy(context_), false);

    callableBoxCapture_ = llvm::StructType::get(llvm::Type::getInt8PtrTy(context_),
                                                captureDeinit()->getPointerTo(), callable());

    auto compiler = codeGenerator_->compiler();
    compiler->sInteger->createUnspecificReification().type = llvm::Type::getInt64Ty(context_);
    compiler->sReal->createUnspecificReification().type = llvm::Type::getDoubleTy(context_);
    compiler->sBoolean->createUnspecificReification().type = llvm::Type::getInt1Ty(context_);
    compiler->sMemory->createUnspecificReification().type = llvm::Type::getInt8PtrTy(context_);
    compiler->sByte->createUnspecificReification().type = llvm::Type::getInt8Ty(context_);

    tbaaRoot_ = mdBuilder_.createTBAARoot("something");
}

LLVMTypeHelper::~LLVMTypeHelper() = default;

void LLVMTypeHelper::withReificationContext(ReificationContext context, std::function<void ()> function) {
    auto ptr = std::make_unique<ReificationContext>(std::move(context));
    std::swap(ptr, reifiContext_);
    function();
    reifiContext_ = std::move(ptr);
}

llvm::StructType* LLVMTypeHelper::llvmTypeForCapture(const Capture &capture, llvm::Type *thisType, bool escaping) {
    std::vector<llvm::Type *> types { llvm::Type::getInt8PtrTy(context_), captureDeinit_->getPointerTo() };
    if (capture.capturesSelf()) {
        types.emplace_back(thisType);
    }
    std::transform(capture.captures.begin(), capture.captures.end(), std::back_inserter(types),
                   [this, escaping](auto &capture) {
        return escaping ? llvmTypeFor(capture.type) : llvmTypeFor(capture.type)->getPointerTo();
    });
    return llvm::StructType::get(context_, types);
}

llvm::ArrayType* LLVMTypeHelper::multiprotocolConformance(const Type &type) {
    return llvm::ArrayType::get(protocolConformance()->getPointerTo(), type.protocols().size());
}

llvm::FunctionType* LLVMTypeHelper::functionTypeFor(Function *function) {
    std::vector<llvm::Type *> args;
    if (function->isClosure()) {
        args.emplace_back(llvm::Type::getInt8PtrTy(context_));
    }
    else if (hasThisArgument(function)) {
        args.emplace_back(llvmTypeFor(function->typeContext().calleeType()));
    }
    std::transform(function->parameters().begin(), function->parameters().end(), std::back_inserter(args), [this](auto &arg) {
        return llvmTypeFor(arg.type->type());
    });
    if (function->functionType() == FunctionType::ObjectInitializer ||
        function->functionType() == FunctionType::ValueTypeInitializer) {
        if (storesGenericArgs(function->typeContext().calleeType())) {
            args.emplace_back(typeDescription_->getPointerTo());
        }
    }
    if (!function->genericParameters().empty()) {
        args.emplace_back(typeDescription_->getPointerTo());
    }
    if (function->errorProne()) {
        args.emplace_back(llvmTypeFor(function->errorType()->type())->getPointerTo());
    }
    llvm::Type *returnType;
    if (function->functionType() == FunctionType::ObjectInitializer) {
        auto init = dynamic_cast<Initializer *>(function);
        returnType = llvmTypeFor(init->constructedType(init->typeContext().calleeType()));
    }
    else {
        returnType = llvmTypeFor(function->returnType()->type());
    }
    return llvm::FunctionType::get(returnType, args, false);
}

llvm::Type* LLVMTypeHelper::box() const {
    return box_;
}

bool LLVMTypeHelper::isDereferenceable(const Type &type) const {
    return ((type.type() == TypeType::Class || type.type() == TypeType::Someobject) &&
            type.storageType() != StorageType::Box) || type.isReference();
}

bool LLVMTypeHelper::isRemote(const Type &type) {
    return codeGenerator_->querySize(llvmTypeFor(type)) > kBoxSize;
}

bool LLVMTypeHelper::storesGenericArgs(const Type &type) const {
    if (type.type() == TypeType::Class) {
        return !type.typeDefinition()->genericParameters().empty() || type.klass()->offset() > 0;
    }
    if (type.type() == TypeType::ValueType) {
        return !type.typeDefinition()->genericParameters().empty();
    }
    return false;
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
        case StorageType::PointerOptional:
            return llvmTypeFor(type.optionalType());
        case StorageType::Simple:
            return getSimpleType(type);
    }
}

llvm::Type* LLVMTypeHelper::getSimpleType(const Type &type) {
    switch (type.type()) {
        case TypeType::Callable:
            return callable_;
        case TypeType::TypeAsValue:
            if (type.typeOfTypeValue().type() == TypeType::Class) {
                return classInfoType_->getPointerTo();
            }
            return llvm::StructType::get(context_);
        case TypeType::Enum:
            return llvm::Type::getInt64Ty(context_);
        case TypeType::Someobject:
            return someobjectPtr_;
        case TypeType::NoReturn:
            return llvm::Type::getVoidTy(context_);
        case TypeType::ValueType:
            return llvmTypeForTypeDefinition(type);
        case TypeType::Class:
            return llvmTypeForTypeDefinition(type)->getPointerTo();
        default:
            throw std::logic_error("No LLVM type could be established.");
    }
}

llvm::Type* LLVMTypeHelper::llvmTypeForTypeDefinition(const Type &type) {
    auto &reification = type.typeDefinition()->reificationFor(type.genericArguments());
    if (reification.type != nullptr) {
        return reification.type;
    }

    auto structType = llvm::StructType::create(context_, mangleTypeName(type));
    reification.type = structType;

    std::vector<llvm::Type *> types;
    if (type.is<TypeType::Class>()) {
        types.emplace_back(llvm::Type::getInt8PtrTy(context_));
        types.emplace_back(classInfoType_->getPointerTo());
    }

    if (storesGenericArgs(type)) {
        types.emplace_back(typeDescription_->getPointerTo());
    }

    for (auto &ivar : type.typeDefinition()->instanceVariables()) {
        types.emplace_back(llvmTypeFor(ivar.type->type()));
    }

    structType->setBody(types);  // for self referencing types
    return structType;
}

llvm::StructType* LLVMTypeHelper::managable(llvm::Type *type) const {
    return llvm::StructType::get(context_, { llvm::Type::getInt8PtrTy(context_), type });
}

llvm::MDNode* LLVMTypeHelper::tbaaNodeFor(const Type &type, bool classAsStruct) {
    if (type.is<TypeType::Enum>() || (type.is<TypeType::ValueType>() && type.valueType()->isPrimitive())) {
        return mdBuilder_.createTBAAScalarTypeNode(mangleTypeName(type), tbaaRoot_);
    }
    if (!classAsStruct && type.is<TypeType::Class>()) {
        auto superclass = type.klass()->superclass();
        auto super = superclass != nullptr ? tbaaNodeFor(type.klass()->superType()->type(), false) : tbaaRoot_;
        return mdBuilder_.createTBAAScalarTypeNode(mangleTypeName(type) + ".ref", super);
    }
    if (type.is<TypeType::ValueType>() || type.is<TypeType::Class>()) {
        std::vector<std::pair<llvm::MDNode*, uint64_t>> elements;
        uint64_t offset = 0;
        for (auto &ivar : type.typeDefinition()->instanceVariables()) {
            elements.emplace_back(tbaaNodeFor(ivar.type->type(), false), offset++);
        }
        return mdBuilder_.createTBAAStructTypeNode(mangleTypeName(type), elements);
    }
    return tbaaRoot_;
}

bool LLVMTypeHelper::shouldAddTbaa(const Type &loadStoreType) const {
    return loadStoreType.is<TypeType::Class>() || loadStoreType.is<TypeType::Enum>() ||
        (loadStoreType.is<TypeType::ValueType>() && loadStoreType.valueType()->isPrimitive());
}

}  // namespace EmojicodeCompiler
