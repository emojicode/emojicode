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
#include <limits>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Package *package, bool optimize)
        : package_(package),
          module_(std::make_unique<llvm::Module>(package->name(), context())),
          typeHelper_(context(), this), declarator_(this), protocolsTableGenerator_(this),
          optimizationManager_(module_.get(), optimize) {}

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
    if (type.type() == TypeType::Class || type.type() == TypeType::TypeAsValue) {  // TODO: better
        return declarator_.boxInfoForObjects();
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
    optimizationManager_.initialize();

    for (auto package : package_->dependencies()) {
        declarator_.declareImportedPackageSymbols(package);
    }

    for (auto &protocol : package_->protocols()) {
        createProtocolFunctionTypes(protocol.get());
    }
    for (auto &valueType : package_->valueTypes()) {
        valueType->eachFunction([this](Function *function) {
            if (!function->requiresCopyReification()) {
                function->createUnspecificReification();
            }
            declarator_.declareLlvmFunction(function);
        });
        protocolsTableGenerator_.createProtocolsTable(Type(valueType.get()));
    }
    for (auto &klass : package_->classes()) {
        VTCreator(klass.get(), declarator_).build();
        protocolsTableGenerator_.createProtocolsTable(Type(klass.get()));
        createClassInfo(klass.get());
    }
    for (auto &function : package_->functions()) {
        function->createUnspecificReification();
        declarator_.declareLlvmFunction(function.get());
    }

    generateFunctions();

    optimizationManager_.optimize(module());
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
    }
    for (auto &klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            generateFunction(function);
        });
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
            optimizationManager_.optimize(reification.entity.function);
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
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classInfo(),
                                                 { superclass, virtualTable, klass->boxInfo() });
    auto info = new llvm::GlobalVariable(*module(), typeHelper_.classInfo(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

}  // namespace EmojicodeCompiler
