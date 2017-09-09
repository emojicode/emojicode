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
#include "../Compiler.hpp"
#include "../CompilerError.hpp"
#include "../EmojicodeCompiler.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Class.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeDefinition.hpp"
#include "../Types/ValueType.hpp"
#include "../Functions/ProtocolFunction.hpp"
#include "FunctionCodeGenerator.hpp"
#include "Mangler.hpp"
#include <algorithm>
#include <limits>
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
    auto ft = typeHelper().functionTypeFor(function);
    auto name = function->isExternal() ? function->externalName() : mangleFunctionName(function);
    auto llvmFunction = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module());
    function->setLlvmFunction(llvmFunction);
}

void CodeGenerator::generate(const std::string &outPath) {
    declareRunTime();

    for (auto package : package_->dependencies()) {
        declareImportedPackageSymbols(package);
    }

    declarePackageSymbols();
    generateFunctions();

    llvm::verifyModule(*module(), &llvm::outs());
    llvm::outs() << *module();
    emit(outPath);
}

void CodeGenerator::declarePackageSymbols() {
    for (auto valueType : package_->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
        createProtocolsTable(valueType);
    }
    for (auto klass : package_->classes()) {
        klass->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
        createProtocolsTable(klass);
        createClassInfo(klass);
    }
    for (auto function : package_->functions()) {
        declareLlvmFunction(function);
    }
    for (auto protocol : package_->protocols()) {
        createProtocolFunctionTypes(protocol);
    }
}

void CodeGenerator::declareImportedPackageSymbols(Package *package) {
    for (auto valueType : package->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
    }
    for (auto klass : package->classes()) {
        klass->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
    }
    for (auto function : package->functions()) {
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

void CodeGenerator::createProtocolFunctionTypes(Protocol *protocol) {
    for (auto method : protocol->methodList()) {
        dynamic_cast<ProtocolFunction *>(method)->setLlvmFunctionType(typeHelper().functionTypeFor(method));
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
            functions[function->vti()] = function->llvmFunction();
        }
    });

    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context()), klass->virtualTableIndicesCount());
    auto virtualTable = new llvm::GlobalVariable(*module(), type, true,
                                                 llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                 llvm::ConstantArray::get(type, functions));
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classMeta(), std::vector<llvm::Constant *> {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(context()), 0), virtualTable, klass->protocolsTable()
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

    auto initializer = llvm::ConstantStruct::get(typeHelper_.valueTypeMeta(), valueType->protocolsTable());
    auto meta = new llvm::GlobalVariable(*module(), typeHelper_.valueTypeMeta(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleValueTypeMetaName(type));
    valueType->addValueTypeMetaFor(type.genericArguments(), meta);
    return meta;
}

void CodeGenerator::createProtocolsTable(TypeDefinition *typeDef) {
    llvm::Constant* init;
    if (typeDef->protocols().size() > 0) {
        auto tables = createProtocolVirtualTables(typeDef);

        auto arrayType = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context_)->getPointerTo(),
                                              tables.tables.size());
        auto protocols = new llvm::GlobalVariable(*module(), arrayType, true,
                                                  llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                  llvm::ConstantArray::get(arrayType, tables.tables));

        init = llvm::ConstantStruct::get(typeHelper_.protocolsTable(), std::vector<llvm::Constant *>{
            protocols,
            llvm::ConstantInt::get(llvm::Type::getInt16Ty(context()), tables.min),
            llvm::ConstantInt::get(llvm::Type::getInt16Ty(context()), tables.max)
        });
    }
    else {
        init = llvm::ConstantAggregateZero::get(typeHelper_.protocolsTable());
    }
    typeDef->setProtocolsTable(init);
}

CodeGenerator::ProtocolVirtualTables CodeGenerator::createProtocolVirtualTables(TypeDefinition *typeDef) {
    std::vector<std::pair<llvm::Constant *, size_t>> protocolVirtualTablesUnordered;

    size_t min = std::numeric_limits<size_t>::max();
    size_t max = 0;

    for (auto &protocol : typeDef->protocols()) {
        auto index = protocol.protocol()->index();
        if (index < min) {
            min = index;
        }
        if (index > max) {
            max = index;
        }
        protocolVirtualTablesUnordered.emplace_back(createProtocolVirtualTable(typeDef, protocol.protocol()), index);
    }

    auto type = llvm::UndefValue::get(llvm::Type::getInt8PtrTy(context_)->getPointerTo());
    std::vector<llvm::Constant *> protocolVirtualTables(max - min + 1, type);
    for (auto &pair : protocolVirtualTablesUnordered) {
        protocolVirtualTables[pair.second - min] = pair.first;
    }

    return CodeGenerator::ProtocolVirtualTables(protocolVirtualTables, max, min);
}

llvm::GlobalVariable* CodeGenerator::createProtocolVirtualTable(TypeDefinition *typeDef, Protocol *protocol) {
    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context_), protocol->methodList().size());

    std::vector<llvm::Constant *> virtualTable;
    virtualTable.resize(protocol->methodList().size());

    for (auto protocolMethod : protocol->methodList()) {
        virtualTable[protocolMethod->vti()] = typeDef->lookupMethod(protocolMethod->name())->llvmFunction();
    }

    auto array = llvm::ConstantArray::get(type, virtualTable);
    return new llvm::GlobalVariable(*module(), type, true, llvm::GlobalValue::LinkageTypes::ExternalLinkage, array);
}

void CodeGenerator::declareRunTime() {
    runTimeNew_ = declareRunTimeFunction("ejcAlloc", llvm::Type::getInt8PtrTy(context()),
                                          llvm::Type::getInt64Ty(context()));
    errNoValue_ = declareRunTimeFunction("ejcErrNoValue", llvm::Type::getVoidTy(context()), std::vector<llvm::Type *> {
        llvm::Type::getInt64Ty(context()), llvm::Type::getInt64Ty(context())
    });

    classValueTypeMeta_ = new llvm::GlobalVariable(*module(), typeHelper_.valueTypeMeta(), true,
                                                   llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                   nullptr, "ejcRunTimeClassValueTypeMeta");
}

llvm::Function* CodeGenerator::declareRunTimeFunction(const char *name, llvm::Type *returnType,
                                           llvm::ArrayRef<llvm::Type *> args) {
    auto ft = llvm::FunctionType::get(returnType, args, false);
    return llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module());
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
