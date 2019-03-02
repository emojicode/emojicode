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
#include "Declarator.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Functions/Initializer.hpp"
#include "Mangler.hpp"
#include "OptimizationManager.hpp"
#include "Package/Package.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "ReificationContext.hpp"
#include "StringPool.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include <algorithm>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Compiler *compiler)
        : compiler_(compiler),
          typeHelper_(context(), this),
          pool_(std::make_unique<StringPool>(this)),
          protocolsTableGenerator_(std::make_unique<ProtocolsTableGenerator>(this)) {}

CodeGenerator::~CodeGenerator() = default;

Compiler* CodeGenerator::compiler() const {
    return compiler_;
}

uint64_t CodeGenerator::querySize(llvm::Type *type) const {
    return module()->getDataLayout().getTypeAllocSize(type);
}

llvm::Constant *CodeGenerator::boxInfoFor(const Type &type) {
    if (type.type() == TypeType::Class) {
        return declarator_->boxInfoForObjects();
    }
    if (type.type() == TypeType::Callable) {
        return declarator_->boxInfoForCallables();
    }
    if (type.type() == TypeType::TypeAsValue) {
        return compiler()->sInteger->boxInfo();
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

void CodeGenerator::prepareModule(Package *package, bool optimize) {
    module_ = std::make_unique<llvm::Module>(package->name(), context());
    optimizationManager_ = std::make_unique<OptimizationManager>(module_.get(), optimize);
    declarator_ = std::make_unique<Declarator>(this);

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

void CodeGenerator::generate(Package *package, const std::string &outPath, bool printIr, bool optimize) {
    prepareModule(package, optimize);
    optimizationManager_->initialize();

    buildClassObjectBoxInfo();
    buildCallableBoxInfo();

    for (auto package : compiler()->importedPackages()) {
        declareAndCreate(package, true);
    }
    declareAndCreate(package, false);

    for (auto package : compiler()->importedPackages()) {
        generateFunctions(package, true);
    }
    generateFunctions(package, false);

    optimizationManager_->optimize(module());
    emitModule(outPath, printIr);
}

void CodeGenerator::declareAndCreate(Package *package, bool imported) {
    for (auto &protocol : package->protocols()) {
        createProtocolFunctionTypes(protocol.get());
    }
    for (auto &valueType : package->valueTypes()) {
        valueType->createUnspecificReification();
        valueType->eachFunction([this, imported](Function *function) {
            if (!imported && !function->requiresCopyReification()) {
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

        if (!imported) {
            valueType->setBoxInfo(declarator().declareBoxInfo(mangleBoxInfoName(Type(valueType.get()))));
            valueType->setBoxRetainRelease(buildBoxRetainRelease(Type(valueType.get())));
            protocolsTableGenerator_->generate(Type(valueType.get()));
            valueType->boxInfo()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
                protocolsTableGenerator_->createProtocolTable(valueType.get()),
                valueType->boxRetainRelease().first, valueType->boxRetainRelease().second
            }));
        }
        else {
            protocolsTableGenerator_->declareImported(Type(valueType.get()));
        }
    }
    for (auto &klass : package->classes()) {
        klass->createUnspecificReification();
        VTCreator(klass.get(), *declarator_).build();
        if (!imported) {
            klass->setBoxRetainRelease(classObjectRetainRelease_);
            protocolsTableGenerator_->generate(Type(klass.get()));
            createClassInfo(klass.get());
        }
        else {
            protocolsTableGenerator_->declareImported(Type(klass.get()));
            declarator().declareImportedClassInfo(klass.get());
        }
    }
    for (auto &function : package->functions()) {
        if (!imported) {
            function->createUnspecificReification();
        }
        declarator_->declareLlvmFunction(function.get());
    }
}

void CodeGenerator::emitModule(const std::string &outPath, bool printIr) {
    llvm::legacy::PassManager pass;

    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code errorCode;
    llvm::raw_fd_ostream dest(outPath, errorCode, llvm::sys::fs::F_None);
    if (targetMachine_->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        puts("TargetMachine can't emit a file of this type");
    }
    pass.add(llvm::createVerifierPass(false));
    pass.add(llvm::createPromoteMemoryToRegisterPass());
    if (printIr) {
        pass.add(llvm::createStripDeadPrototypesPass());
        pass.add(llvm::createPrintModulePass(llvm::outs()));
    }
    pass.run(*module());
    dest.flush();
}

void CodeGenerator::generateFunctions(Package *package, bool imported) {
    for (auto &valueType : package->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            generateFunction(function);
        });
        if (!imported && valueType->isManaged()) {
            generateFunction(valueType->deinitializer());
            generateFunction(valueType->copyRetain());
        }
    }
    for (auto &klass : package->classes()) {
        klass->eachFunction([this](auto *function) {
            generateFunction(function);
        });
        if (!imported) {
            generateFunction(klass->deinitializer());
        }
    }
    for (auto &function : package->functions()) {
        generateFunction(function.get());
    }
}

void CodeGenerator::generateFunction(Function *function) {
    if (!function->isExternal()) {
        function->eachReification([this, function](auto &reification) {
            typeHelper_.withReificationContext(ReificationContext(*function, reification), [&] {
                FunctionCodeGenerator(function, reification.entity.function, this).generate();
            });
            optimizationManager_->optimize(reification.entity.function);
        });
    }
}

void CodeGenerator::createProtocolFunctionTypes(Protocol *protocol) {
    size_t tableIndex = 0;
    for (auto function : protocol->methodList()) {
        function->createUnspecificReification();
        function->eachReification([this, function, &tableIndex](auto &reification) {
            typeHelper_.withReificationContext(ReificationContext(*function, reification), [&] {
                reification.entity.setFunctionType(typeHelper().functionTypeFor(function));
                reification.entity.setVti(tableIndex++);
            });
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
    auto gep = llvm::ConstantExpr::getInBoundsGetElementPtr(virtualTable->getType()->getElementType(), virtualTable,
                                                 llvm::ArrayRef<llvm::Constant *>{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)
    });
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classInfo(), { superclass, gep, protocolTable });
    auto info = new llvm::GlobalVariable(*module(), typeHelper_.classInfo(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

std::pair<llvm::Function*, llvm::Function*> CodeGenerator::buildBoxRetainRelease(const Type &type) {
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
            auto containedType = typeHelper().llvmTypeFor(type);
            auto mngType = typeHelper().managable(containedType);

            auto objPtr = releaseFg.buildGetBoxValuePtrAfter(release->args().begin(), mngType->getPointerTo(),
                                                             containedType->getPointerTo());
            auto remotePtr = releaseFg.builder().CreateLoad(objPtr);
            releaseFg.release(releaseFg.managableGetValuePtr(remotePtr), type);
            releaseFg.release(releaseFg.builder().CreateBitCast(remotePtr, llvm::Type::getInt8PtrTy(context_)),
                              Type(compiler()->sMemory));

            auto objPtrRetain = retainFg.buildGetBoxValuePtrAfter(retain->args().begin(), mngType->getPointerTo(),
                                                                  containedType->getPointerTo());
            auto remotePtrRetain = retainFg.builder().CreateLoad(objPtrRetain);
            retainFg.retain(retainFg.managableGetValuePtr(remotePtrRetain), type);
            retainFg.retain(remotePtrRetain, Type(compiler()->sMemory));
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
    return std::make_pair(retain, release);
}

void CodeGenerator::buildClassObjectBoxInfo() {
    auto klass = compiler()->sString;
    llvm::Function *retain, *release;
    std::tie(retain, release) = classObjectRetainRelease_ = buildBoxRetainRelease(Type(klass));
    retain->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    release->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    retain->setName("class.boxRetain");
    release->setName("class.boxRelease");

    declarator().boxInfoForObjects()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
        llvm::ConstantPointerNull::get(typeHelper().protocolConformanceEntry()->getPointerTo()),
        retain, release
    }));
    declarator().boxInfoForObjects()->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
}

void CodeGenerator::buildCallableBoxInfo() {
    llvm::Function *retain, *release;
    std::tie(retain, release) = buildBoxRetainRelease(Type(Type::noReturn(), {}));
    retain->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    release->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    retain->setName("callable.boxRetain");
    release->setName("callable.boxRelease");

    declarator().boxInfoForCallables()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
        llvm::ConstantPointerNull::get(typeHelper().protocolConformanceEntry()->getPointerTo()),
        retain, release
    }));
    declarator().boxInfoForCallables()->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
}

}  // namespace EmojicodeCompiler
