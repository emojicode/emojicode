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
#include "FnCodeGenerator.hpp"
#include "Mangler.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Package *package) : package_(package) {
    module_ = std::make_unique<llvm::Module>(package->name(), context());

    classMetaType_ = llvm::StructType::create(std::vector<llvm::Type *> {
        llvm::Type::getInt64Ty(context()), llvm::Type::getInt8PtrTy(context())->getPointerTo(),
    }, "classMeta");

    types_.emplace(Type::noReturn(), llvm::Type::getVoidTy(context()));
    types_.emplace(Type::integer(), llvm::Type::getInt64Ty(context()));
    types_.emplace(Type::symbol(), llvm::Type::getInt32Ty(context()));
    types_.emplace(Type::doubl(), llvm::Type::getDoubleTy(context()));
    types_.emplace(Type::boolean(), llvm::Type::getInt1Ty(context()));
}

llvm::Type* CodeGenerator::llvmTypeForType(Type type) {
    llvm::Type *llvmType = nullptr;

    if (type.meta()) {
        return classMetaType_->getPointerTo();
    }

    if (type.optional()) {
        type.setOptional(false);
        std::vector<llvm::Type *> types{ llvm::Type::getInt1Ty(context()), llvmTypeForType(type) };
        llvmType = llvm::StructType::get(context(), types);
    }
    else {
        auto it = types_.find(type);
        if (it != types_.end()) {
            llvmType = it->second;
        }
        if (llvmType == nullptr && (type.type() == TypeType::ValueType || type.type() == TypeType::Class)) {
            llvmType = createLlvmTypeForTypeDefinition(type);
        }
        if (llvmType != nullptr && type.type() == TypeType::Class) {
            llvmType = llvmType->getPointerTo();
        }
    }

    if (type.type() == TypeType::Error) {
        std::vector<llvm::Type *> types{ llvm::Type::getInt1Ty(context()),
                                         llvmTypeForType(type.genericArguments()[1]) };
        llvmType = llvm::StructType::get(context(), types);
    }

    if (llvmType == nullptr) {
        return llvm::Type::getVoidTy(context());
    }

    return type.isReference() ? llvmType->getPointerTo() : llvmType;
}

llvm::Type* CodeGenerator::createLlvmTypeForTypeDefinition(const Type &type) {
    std::vector<llvm::Type *> types;

    if (type.type() == TypeType::Class) {
        types.emplace_back(classMetaType_->getPointerTo());
    }

    for (auto &ivar : type.typeDefinition()->instanceVariables()) {
        types.emplace_back(llvmTypeForType(ivar.type));
    }

    auto llvmType = llvm::StructType::create(context(), types, mangleTypeName(type));
    types_.emplace(type, llvmType);
    return llvmType;
}

void CodeGenerator::declareLlvmFunction(Function *function) {
    std::vector<llvm::Type *> args;
    if (isSelfAllowed(function->functionType())) {
        args.emplace_back(llvmTypeForType(function->typeContext().calleeType()));
    }
    std::transform(function->arguments.begin(), function->arguments.end(), std::back_inserter(args), [this](auto &arg) {
        return llvmTypeForType(arg.type);
    });
    llvm::Type *returnType;
    if (function->functionType() == FunctionType::ObjectInitializer) {
        auto init = dynamic_cast<Initializer *>(function);
        returnType = llvmTypeForType(init->constructedType(init->typeContext().calleeType()));
    }
    else {
        returnType = llvmTypeForType(function->returnType);
    }
    auto ft = llvm::FunctionType::get(returnType, args, false);
    auto llvmFunction = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                               mangleFunctionName(function), module());
    function->setLlvmFunction(llvmFunction);
}

void CodeGenerator::generate(const std::string &outPath) {
    declareRunTime();

    for (auto valueType : package_->valueTypes()) {
        
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

    for (auto function : package_->functions()) {
        generateFunction(function);
    }
    for (auto klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            generateFunction(function);
        });
    }

    llvm::verifyModule(*module(), &llvm::outs());
    module()->dump();
    emit(outPath);
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
    auto initializer = llvm::ConstantStruct::get(classMetaType_, std::vector<llvm::Constant *> {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), 0), virtualTable,
    });
    auto info = new llvm::GlobalVariable(*module(), classMetaType_, true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassMetaName(klass));

    klass->virtualTable() = std::move(functions);
    klass->setClassInfo(info);
}

void CodeGenerator::generateFunction(Function *function) {
    FnCodeGenerator(function, this).generate();
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
    std::string Error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    module()->setDataLayout(targetMachine->createDataLayout());
    module()->setTargetTriple(targetTriple);

    llvm::legacy::PassManager pass;

    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code EC;
    llvm::raw_fd_ostream dest(outPath, EC, llvm::sys::fs::F_None);
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
