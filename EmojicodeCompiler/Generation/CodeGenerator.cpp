//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/raw_ostream.h>
#include "CodeGenerator.hpp"
#include "../Application.hpp"
#include "../CompilerError.hpp"
#include "../EmojicodeCompiler.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Class.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeDefinition.hpp"
#include "../Types/ValueType.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Mangler.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Package *package) : package_(package) {
    module_ = std::make_unique<llvm::Module>(package->name(), context());
}

llvm::Value* CodeGenerator::optionalValue() {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()), 1);
}

llvm::Value* CodeGenerator::optionalNoValue() {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context()), 0);
}

void CodeGenerator::declareLlvmFunction(Function *function) {
    std::vector<llvm::Type *> args;
    if (isSelfAllowed(function->functionType())) {
        args.emplace_back(typeHelper_.llvmTypeFor(function->typeContext().calleeType()));
    }
    std::transform(function->arguments.begin(), function->arguments.end(), std::back_inserter(args), [this](auto &arg) {
        return typeHelper_.llvmTypeFor(arg.type);
    });
    llvm::Type *returnType;
    if (function->functionType() == FunctionType::ObjectInitializer) {
        auto init = dynamic_cast<Initializer *>(function);
        returnType = typeHelper_.llvmTypeFor(init->constructedType(init->typeContext().calleeType()));
    }
    else {
        returnType = typeHelper_.llvmTypeFor(function->returnType);
    }
    auto ft = llvm::FunctionType::get(returnType, args, false);
    auto name = function->isExternal() ? function->externalName() : mangleFunctionName(function);
    auto llvmFunction = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module());
    function->setLlvmFunction(llvmFunction);
}

void CodeGenerator::generate(const std::string &outPath) {
    declareRunTime();

    declarePackageSymbols();
    generateFunctions();

    llvm::verifyModule(*module(), &llvm::outs());
    module()->dump();
    emit(outPath);
}

void CodeGenerator::declarePackageSymbols() {
    classValueTypeMeta_ = new llvm::GlobalVariable(*module(), typeHelper_.valueTypeMeta(), true,
                                                   llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                   llvm::Constant::getNullValue(typeHelper_.valueTypeMeta()),
                                                   "classValueTypeMeta");

    for (auto valueType : package_->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
    }
    for (auto klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
        createClassInfo(klass);
    }
    for (auto function : package_->functions()) {
        declareLlvmFunction(function);
    }
}

void CodeGenerator::generateFunctions() {
    for (auto valueType : package_->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            generateFunction(function);
        });
    }
    for (auto function : package_->functions()) {
        generateFunction(function);
    }
    for (auto klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            generateFunction(function);
        });
    }
}

void CodeGenerator::generateFunction(Function *function) {
    if (!function->isExternal()) {
        FunctionCodeGenerator(function, this).generate();
    }
}

void CodeGenerator::createClassInfo(Class *klass) {
    std::vector<llvm::Constant *> functions;
    functions.resize(klass->virtualTableIndicesCount());

    if (auto superclass = klass->superclass()) {
        std::copy(superclass->virtualTable().begin(), superclass->virtualTable().end(), functions.begin());
    }

    klass->eachFunction([&functions, this] (Function *function) {
        if (function->hasVti()) {
            functions[function->getVti()] = function->llvmFunction();
        }
    });

    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context()), klass->virtualTableIndicesCount());
    auto virtualTable = new llvm::GlobalVariable(*module(), type, true,
                                                 llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                 llvm::ConstantArray::get(type, functions));
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classMeta(), std::vector<llvm::Constant *> {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), 0), virtualTable,
    });
    auto meta = new llvm::GlobalVariable(*module(), typeHelper_.classMeta(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassMetaName(klass));

    klass->virtualTable() = std::move(functions);
    klass->setClassMeta(meta);
}

llvm::GlobalVariable* CodeGenerator::valueTypeMetaFor(const Type &type) {
    if (type.type() == TypeType::Class) {
        return classValueTypeMeta_;
    }

    if (auto meta = type.valueType()->valueTypeMetaFor(type.genericArguments())) {
        return meta;
    }

    auto valueType = type.valueType();

    auto initializer = llvm::ConstantStruct::get(typeHelper_.valueTypeMeta(), std::vector<llvm::Constant *> {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), 0),
    });
    auto meta = new llvm::GlobalVariable(*module(), typeHelper_.valueTypeMeta(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleValueTypeMetaName(type));
    valueType->addValueTypeMetaFor(type.genericArguments(), meta);
    return meta;
}

void CodeGenerator::declareRunTime() {
    std::vector<llvm::Type *> args{ llvm::Type::getInt64Ty(context()) };
    auto ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(context()), args, false);
    runTimeNew_ = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "ejcRunTimeAlloc", module());
}

void CodeGenerator::emit(const std::string &outPath) {
    setUpPassManager();

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
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    module()->setDataLayout(targetMachine->createDataLayout());
    module()->setTargetTriple(targetTriple);

    llvm::legacy::PassManager pass;

    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code errorCode;
    llvm::raw_fd_ostream dest(outPath, errorCode, llvm::sys::fs::F_None);
    if (targetMachine->addPassesToEmitFile(pass, dest, fileType)) {
        puts( "TargetMachine can't emit a file of this type" );
    }
    pass.run(*module());
    dest.flush();
}

void CodeGenerator::setUpPassManager() {
    passManager_ = std::make_unique<llvm::legacy::FunctionPassManager>(module());
//
//    // Provide basic AliasAnalysis support for GVN.
//    TheFPM.add(createBasicAliasAnalysisPass());
//    // Do simple "peephole" optimizations and bit-twiddling optzns.
//    TheFPM.add(createInstructionCombiningPass());
//    // Reassociate expressions.
//    TheFPM.add(createReassociatePass());
//    // Eliminate Common SubExpressions.
//    TheFPM.add(createGVNPass());
//    // Simplify the control flow graph (deleting unreachable blocks, etc).
//    TheFPM.add(createCFGSimplificationPass());
//
//    TheFPM.doInitialization();
}

}  // namespace EmojicodeCompiler
