//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CodeGenerator.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Functions/Initializer.hpp"
#include "Mangler.hpp"
#include "Package/Package.hpp"
#include "ReificationContext.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "OptimizationManager.hpp"
#include "Declarator.hpp"
#include "StringPool.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <algorithm>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Package *package, bool optimize)
        : package_(package),
          module_(std::make_unique<llvm::Module>(package->name(), context())),
          typeHelper_(context(), this),
          pool_(std::make_unique<StringPool>(this)),
          declarator_(std::make_unique<Declarator>(this)),
          protocolsTableGenerator_(std::make_unique<ProtocolsTableGenerator>(this)),
          optimizationManager_(std::make_unique<OptimizationManager>(module_.get(), optimize)) {}

CodeGenerator::~CodeGenerator() = default;

llvm::Value *CodeGenerator::optionalValue() {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()), 1);
}

llvm::Value *CodeGenerator::optionalNoValue() {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()), 0);
}

uint64_t CodeGenerator::querySize(llvm::Type *type) const {
    return module()->getDataLayout().getTypeAllocSize(type);
}

llvm::Constant *CodeGenerator::boxInfoFor(const Type &type) {
    if (type.type() == TypeType::Class) {
        return declarator_->boxInfoForObjects();
    }
    if (type.type() == TypeType::TypeAsValue) {
        return package_->compiler()->sInteger->boxInfo();
    }
    assert(type.type() == TypeType::ValueType || type.type() == TypeType::Enum);
    return type.valueType()->boxInfo();
}

llvm::Constant *CodeGenerator::protocolIdentifierFor(const Type &type) {
    auto unboxedType = type.unboxed();
    auto it = protocolIds_.find(unboxedType);
    if (it != protocolIds_.end()) {
        return it->second;
    }

    auto id = new llvm::GlobalVariable(*module_, llvm::Type::getInt1Ty(context_), true,
                                       llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage,
                                       llvm::Constant::getNullValue(llvm::Type::getInt1Ty(context_)),
                                       mangleProtocolIdentifier(unboxedType));

    protocolIds_.emplace(unboxedType, id);
    return id;
}

void CodeGenerator::prepareModule() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    targetMachine_ = target->createTargetMachine(targetTriple, cpu, features, opt, llvm::Reloc::PIC_);

    module()->setDataLayout(targetMachine_->createDataLayout());
    module()->setTargetTriple(targetTriple);
}

void CodeGenerator::generate(const std::string &outPath, bool printIr) {
    prepareModule();
    optimizationManager_->initialize();

    for (auto package : package_->dependencies()) {
        declarator_->declareImportedPackageSymbols(package);
    }

    buildObjectBoxRetainRelease();

    for (auto &protocol : package_->protocols()) {
        createProtocolFunctionTypes(protocol.get());
    }
    for (auto &valueType : package_->valueTypes()) {
        valueType->eachFunction([this](Function *function) {
            if (!function->requiresCopyReification()) {
                function->createUnspecificReification();
            }
            declarator_->declareLlvmFunction(function);
        });
        if (valueType->isManaged()) {
            valueType->deinitializer()->createUnspecificReification();
            declarator_->declareLlvmFunction(valueType->deinitializer());
            valueType->copyRetain()->createUnspecificReification();
            declarator_->declareLlvmFunction(valueType->copyRetain());
        }

        valueType->setBoxInfo(declarator().declareBoxInfo(mangleBoxInfoName(Type(valueType.get()))));
        buildBoxRetainRelease(Type(valueType.get()));
        protocolsTableGenerator_->generate(Type(valueType.get()));
        valueType->boxInfo()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
            protocolsTableGenerator_->createProtocolTable(valueType.get()),
            valueType->boxRetainRelease().first, valueType->boxRetainRelease().second
        }));
    }
    for (auto &klass : package_->classes()) {
        VTCreator(klass.get(), *declarator_).build();
        klass->setBoxRetainRelease(objectRetain_, objectRelease_);
        protocolsTableGenerator_->generate(Type(klass.get()));
        createClassInfo(klass.get());
    }
    for (auto &function : package_->functions()) {
        function->createUnspecificReification();
        declarator_->declareLlvmFunction(function.get());
    }

    generateFunctions();

    optimizationManager_->optimize(module());
    emit(outPath, printIr);
}

void CodeGenerator::emit(const std::string &outPath, bool printIr) {
    llvm::legacy::PassManager pass;

    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code errorCode;
    llvm::raw_fd_ostream dest(outPath, errorCode, llvm::sys::fs::F_None);
    if (targetMachine_->addPassesToEmitFile(pass, dest, fileType)) {
        puts("TargetMachine can't emit a file of this type");
    }
    pass.add(llvm::createVerifierPass(false));
    if (printIr) {
        pass.add(llvm::createStripDeadPrototypesPass());
        pass.add(llvm::createPrintModulePass(llvm::outs()));
    }
    pass.run(*module());
    dest.flush();
}

void CodeGenerator::generateFunctions() {
    for (auto &valueType : package_->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            generateFunction(function);
        });
        if (valueType->isManaged()) {
            generateFunction(valueType->deinitializer());
            generateFunction(valueType->copyRetain());
        }
    }
    for (auto &klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            generateFunction(function);
        });
        generateFunction(klass->deinitializer());
    }
    for (auto &function : package_->functions()) {
        generateFunction(function.get());
    }
}

void CodeGenerator::generateFunction(Function *function) {
    if (!function->isExternal()) {
        function->eachReification([this, function](auto &reification) {
            auto context = ReificationContext(*function, reification);
            typeHelper_.setReificationContext(&context);
            FunctionCodeGenerator(function, reification.entity.function, this).generate();
            typeHelper_.setReificationContext(nullptr);
            optimizationManager_->optimize(reification.entity.function);
        });
    }
}

void CodeGenerator::createProtocolFunctionTypes(Protocol *protocol) {
    size_t tableIndex = 0;
    for (auto function : protocol->methodList()) {
        function->createUnspecificReification();
        function->eachReification([this, function, &tableIndex](auto &reification) {
            auto context = ReificationContext(*function, reification);
            typeHelper().setReificationContext(&context);
            reification.entity.setFunctionType(typeHelper().functionTypeFor(function));
            reification.entity.setVti(tableIndex++);
            typeHelper().setReificationContext(nullptr);
        });
    }
}

void CodeGenerator::createClassInfo(Class *klass) {
    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context()), klass->virtualTable().size());
    auto virtualTable = new llvm::GlobalVariable(*module(), type, true,
                                                 llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                                 llvm::ConstantArray::get(type, klass->virtualTable()));
    llvm::Constant *superclass;
    if (klass->superclass() != nullptr) {
        superclass = klass->superclass()->classInfo();
    }
    else {
        superclass = llvm::ConstantPointerNull::get(typeHelper_.classInfo()->getPointerTo());
    }

    auto protocolTable = protocolsTableGenerator_->createProtocolTable(klass);
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classInfo(), { superclass, virtualTable, protocolTable });
    auto info = new llvm::GlobalVariable(*module(), typeHelper_.classInfo(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

void CodeGenerator::buildBoxRetainRelease(const Type &type) {
    auto release = llvm::Function::Create(typeHelper().boxRetainRelease(),
                                          llvm::GlobalValue::LinkageTypes::ExternalLinkage, mangleBoxRelease(type),
                                          module_.get());
    auto retain = llvm::Function::Create(typeHelper().boxRetainRelease(),
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, mangleBoxRetain(type),
                                         module_.get());

    auto releaseFg = FunctionCodeGenerator(release, this);
    releaseFg.createEntry();
    auto retainFg = FunctionCodeGenerator(retain, this);
    retainFg.createEntry();

    if (type.isManaged()) {
        if (!releaseFg.isManagedByReference(type)) {
            auto objPtr = releaseFg.buildGetBoxValuePtr(release->args().begin(), type);
            releaseFg.release(releaseFg.builder().CreateLoad(objPtr), type);

            auto objPtrRetain = retainFg.buildGetBoxValuePtr(retain->args().begin(), type);
            retainFg.retain(retainFg.builder().CreateLoad(objPtrRetain), type);
        }
        else if (typeHelper().isRemote(type)) {
            auto objPtr = releaseFg.buildGetBoxValuePtr(release->args().begin(), type.referenced());
            auto remotePtr = releaseFg.builder().CreateLoad(objPtr);
            releaseFg.release(remotePtr, type);
            releaseFg.release(releaseFg.builder().CreateBitCast(remotePtr, llvm::Type::getInt8PtrTy(context_)),
                              Type(package()->compiler()->sMemory));

            auto objPtrRetain = retainFg.buildGetBoxValuePtr(retain->args().begin(), type.referenced());
            auto remotePtrRetain = retainFg.builder().CreateLoad(objPtrRetain);
            retainFg.retain(remotePtrRetain, type);
            retainFg.retain(remotePtrRetain, Type(package()->compiler()->sMemory));
        }
        else {
            auto objPtr = releaseFg.buildGetBoxValuePtr(release->args().begin(), type);
            releaseFg.release(objPtr, type);

            auto objPtrRetain = retainFg.buildGetBoxValuePtr(retain->args().begin(), type);
            retainFg.retain(objPtrRetain, type);
        }
    }

    releaseFg.builder().CreateRetVoid();
    retainFg.builder().CreateRetVoid();

    type.typeDefinition()->setBoxRetainRelease(retain, release);
}

void CodeGenerator::buildObjectBoxRetainRelease() {
    auto klass = package_->compiler()->sString;
    buildBoxRetainRelease(Type(klass));
    std::tie(objectRetain_, objectRelease_) = klass->boxRetainRelease();
    objectRetain_->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    objectRelease_->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    objectRetain_->setName("class.boxRetain");
    objectRelease_->setName("class.boxRelease");

    declarator().boxInfoForObjects()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
        llvm::ConstantPointerNull::get(typeHelper().protocolConformanceEntry()->getPointerTo()),
        objectRetain_, objectRelease_
    }));
    declarator().boxInfoForObjects()->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
}

}  // namespace EmojicodeCompiler
