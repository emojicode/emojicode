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
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

CodeGenerator::CodeGenerator(Package *package) : package_(package) {
    module_ = std::make_unique<llvm::Module>(package->name(), context());

    types_.emplace(Type::noReturn(), llvm::Type::getVoidTy(context()));
    types_.emplace(Type::integer(), llvm::Type::getInt64Ty(context()));
    types_.emplace(Type::symbol(), llvm::Type::getInt32Ty(context()));
    types_.emplace(Type::doubl(), llvm::Type::getDoubleTy(context()));
    types_.emplace(Type::boolean(), llvm::Type::getInt1Ty(context()));
}

std::string CodeGenerator::mangleFunctionName(Function *function) {
    std::stringstream stream;
    stream << "fn_";
    if (function->owningType().type() != TypeType::NoReturn) {
        mangleIdentifier(stream, function->owningType().typeDefinition()->name());
        stream << "_";
    }
    mangleIdentifier(stream, function->name());
    return stream.str();
}

void CodeGenerator::mangleIdentifier(std::stringstream &stream, const std::u32string &string) {
    for (auto ch : string) {
        stream << std::hex << ch << "_";
    }
}

std::string CodeGenerator::mangleTypeName(TypeDefinition *typeDef) {
    std::stringstream stream;
    stream << "ty_";
    mangleIdentifier(stream, typeDef->name());
    return stream.str();
}

llvm::Type* CodeGenerator::llvmTypeForType(Type type) {
    llvm::Type *llvmType = nullptr;

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

    for (auto &ivar : type.typeDefinition()->instanceVariables()) {
        types.emplace_back(llvmTypeForType(ivar.type));
    }

    auto name = mangleTypeName(type.typeDefinition());
    auto llvmType = llvm::StructType::create(context(), types, name);
    types_.emplace(type, llvmType);
    return llvmType;
}

void CodeGenerator::createLlvmFunction(Function *function) {
    std::vector<llvm::Type *> args;
    if (isSelfAllowed(function->functionType())) {
        args.emplace_back(llvmTypeForType(function->typeContext().calleeType()));
    }
    std::transform(function->arguments.begin(), function->arguments.end(), std::back_inserter(args), [this](auto &arg) {
        return llvmTypeForType(arg.type);
    });
    auto ft = llvm::FunctionType::get(llvmTypeForType(function->returnType), args, false);
    auto llvmFunction = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                               mangleFunctionName(function), module());
    function->setLlvmFunction(llvmFunction);
}

void CodeGenerator::generate(const std::string &outPath) {
    declareRunTime();

    for (auto valueType : package_->valueTypes()) {
        valueType->prepareForCG();
    }
    for (auto klass : package_->classes()) {
        klass->prepareForCG();
        klass->eachFunction([this](auto *function) {
            createLlvmFunction(function);
        });
    }
    for (auto function : package_->functions()) {
        createLlvmFunction(function);
    }

    for (auto function : package_->functions()) {
        FnCodeGenerator(function, this).generate();
    }

    module()->dump();
    emit(outPath);
}

void CodeGenerator::declareRunTime() {
    std::vector<llvm::Type *> args{ llvm::Type::getInt64Ty(context()) };
    auto ft = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(context()), args, false);
    llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "emojicoreNew", module());
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


//void writeProtocolTable(TypeDefinition *typeDefinition, Writer *writer) {
//    std::vector<Type> usedProtocols;
//    std::copy_if(typeDefinition->protocols().begin(), typeDefinition->protocols().end(),
//                 std::back_inserter(usedProtocols), [](const Type &protocol) {
//        auto methodList = protocol.protocol()->methodList();
//        return std::any_of(methodList.begin(), methodList.end(), [](Function *f){ return f->used(); });
//    });
//
//    writer->writeUInt16(usedProtocols.size());
//    if (!usedProtocols.empty()) {
//        auto biggestPlaceholder = writer->writePlaceholder<uint16_t>();
//        auto smallestPlaceholder = writer->writePlaceholder<uint16_t>();
//
//        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
//        uint_fast16_t biggestProtocolIndex = 0;
//
//        for (const Type &protocol : usedProtocols) {
//            writer->writeUInt16(protocol.protocol()->index);
//
//            if (protocol.protocol()->index > biggestProtocolIndex) {
//                biggestProtocolIndex = protocol.protocol()->index;
//            }
//            if (protocol.protocol()->index < smallestProtocolIndex) {
//                smallestProtocolIndex = protocol.protocol()->index;
//            }
//
//            writer->writeUInt16(protocol.protocol()->methodList().size());
//
//            for (auto method : protocol.protocol()->methodList()) {
//                auto layerName = method->protocolBoxingLayerName(protocol.protocol()->name());
//                Function *clm = typeDefinition->lookupMethod(layerName);
//                if (clm == nullptr) {
//                    clm = typeDefinition->lookupMethod(method->name());
//                }
//                writer->writeUInt16(clm->used() ? clm->getVti() : 0);
//            }
//        }
//
//        biggestPlaceholder.write(biggestProtocolIndex);
//        smallestPlaceholder.write(smallestProtocolIndex);
//    }
//}
//
//void writeClass(const Type &classType, Writer *writer) {
//    auto eclass = classType.eclass();
//
//    writer->writeEmojicodeChar(eclass->name()[0]);
//    if (eclass->superclass() != nullptr) {
//        writer->writeUInt16(eclass->superclass()->index);
//    }
//    else {
//        // If the class does not have a superclass the own index is written.
//        writer->writeUInt16(eclass->index);
//    }
//
//    writer->writeUInt16(eclass->size());
//    writer->writeUInt16(eclass->fullMethodCount());
//    writer->writeByte(eclass->inheritsInitializers() ? 1 : 0);
//    writer->writeUInt16(eclass->fullInitializerCount());
//
//    if (eclass->fullInitializerCount() > 65535) {
//        throw CompilerError(eclass->position(), "More than 65535 initializers in class.");
//    }
//    if (eclass->fullMethodCount() > 65535) {
//        throw CompilerError(eclass->position(), "More than 65535 methods in class.");
//    }
//
//    writer->writeUInt16(eclass->usedMethodCount());
//    writer->writeUInt16(eclass->usedInitializerCount());
//
//    writeUsed(eclass->methodList(), writer);
//    writeUsed(eclass->typeMethodList(), writer);
//    writeUsed(eclass->initializerList(), writer);
//
//    writeProtocolTable(eclass, writer);
//
//    std::vector<ObjectVariableInformation> information;
//    for (auto variable : eclass->instanceScope().map()) {
//        // TODO:    variable.second.type().objectVariableRecords(variable.second.id(), &information);
//    }
//
//    writer->writeUInt16(information.size());
//
//    for (auto info : information) {
//        writer->writeUInt16(info.index);
//        writer->writeUInt16(info.conditionIndex);
//        writer->writeUInt16(static_cast<uint16_t>(info.type));
//    }
//}
//
//void writePackageHeader(Package *pkg, Writer *writer) {
//    if (pkg->requiresBinary()) {
//        size_t l = pkg->name().size() + 1;
//        writer->writeByte(l);
//        writer->writeBytes(pkg->name().c_str(), l);
//
//        writer->writeUInt16(pkg->version().major);
//        writer->writeUInt16(pkg->version().minor);
//    }
//    else {
//        writer->writeByte(0);
//    }
//
//    writer->writeUInt16(pkg->classes().size());
//}
//
//void generateCode(Writer *writer, Application *app) {
//    app->stringPool().pool(std::u32string());
//
//    app->startFlagFunction()->setVtiProvider(&STIProvider::globalStiProvider);
//    app->startFlagFunction()->vtiForUse();
//
//    for (auto &package : app->packagesInOrder()) {
//        for (auto valueType : package->valueTypes()) {
//            valueType->prepareForCG();
//        }
//        for (auto klass : package->classes()) {
//            klass->prepareForCG();
//        }
//    }
//
//    while (!app->compilationQueue.empty()) {
//        Function *function = app->compilationQueue.front();
//        generateCodeForFunction(function);
//        app->compilationQueue.pop();
//    }
//
//    if (app->maxBoxIndetifier() > 2147483647) {
//        throw CompilerError(SourcePosition(0, 0, ""), "More than 2147483647 box identifiers in use.");
//    }
//    if (app->protocolCount() == UINT16_MAX) {
//        throw CompilerError(SourcePosition(0, 0, ""), "You exceeded the limit of 65,536 protocols.");
//    }
//
//    writer->writeUInt16(app->classCount());
//    writer->writeUInt16(STIProvider::functionCount());
//
//    if (app->packagesInOrder().size() > 255) {
//        throw CompilerError(app->packagesInOrder().back()->position(), "You exceeded the maximum of 255 packages.");
//    }
//    writer->writeByte(app->packagesInOrder().size());
//
//    for (auto &pkg : app->packagesInOrder()) {
//        writePackageHeader(pkg.get(), writer);
//
//        for (auto cl : pkg->classes()) {
//            writeClass(Type(cl, false), writer);
//        }
//
//        auto placeholder = writer->writePlaceholder<uint16_t>();
//        placeholder.write(writeUsed(pkg->functions(), writer));
//    }
//
//    uint32_t smallestBoxIdentifier = UINT16_MAX;
//    uint32_t biggestBoxIdentifier = 0;
//    int vtWithProtocolsCount = 0;
//    auto tableSizePlaceholder = writer->writePlaceholder<uint16_t>();
//    auto smallestPlaceholder = writer->writePlaceholder<uint16_t>();
//    auto countPlaceholder = writer->writePlaceholder<uint16_t>();
//    for (auto &package : app->packagesInOrder()) {
//        for (auto vt : package->valueTypes()) {
//            if (!vt->protocols().empty()) {
//                for (auto idPair : vt->genericIds()) {
//                    writer->writeUInt16(idPair.second);
//                    writeProtocolTable(vt, writer);
//                    if (idPair.second < smallestBoxIdentifier) {
//                        smallestBoxIdentifier = idPair.second;
//                    }
//                    if (idPair.second > biggestBoxIdentifier) {
//                        biggestBoxIdentifier = idPair.second;
//                    }
//                    vtWithProtocolsCount++;
//                }
//            }
//        }
//    }
//    countPlaceholder.write(vtWithProtocolsCount);
//    tableSizePlaceholder.write(vtWithProtocolsCount > 0 ? biggestBoxIdentifier - smallestBoxIdentifier + 1 : 0);
//    smallestPlaceholder.write(smallestBoxIdentifier);
//
//    auto binfo = app->boxObjectVariableInformation();
//    writer->writeInstruction(binfo.size());
//    for (auto information : binfo) {
//        writer->writeUInt16(information.size());
//
//        for (auto info : information) {
//            writer->writeUInt16(info.index);
//            writer->writeUInt16(info.conditionIndex);
//            writer->writeUInt16(static_cast<uint16_t>(info.type));
//        }
//    }
//
//    writer->writeUInt16(app->stringPool().strings().size());
//    for (auto string : app->stringPool().strings()) {
//        writer->writeUInt16(string.size());
//
//        for (auto c : string) {
//            writer->writeEmojicodeChar(c);
//        }
//    }
//}

}  // namespace EmojicodeCompiler
