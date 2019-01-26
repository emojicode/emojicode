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
#include "Package/RecordingPackage.hpp"
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

void CodeGenerator::prepareModule(bool optimize) {
    module_ = std::make_unique<llvm::Module>(compiler_->mainPackage()->name(), context());
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

void CodeGenerator::generate(const std::string &outPath, bool printIr, bool optimize) {
    prepareModule(optimize);
    optimizationManager_->initialize();

    for (auto package : compiler()->importedPackages()) {
        declareAndCreate(package, true);
    }
    declareAndCreate(compiler_->mainPackage(), false);

    for (auto package : compiler()->importedPackages()) {
        generateFunctions(package, true);
    }
    generateFunctions(compiler_->mainPackage(), false);

    optimizationManager_->optimize(module());
    emitModule(outPath, printIr);
}

void CodeGenerator::declareAndCreate(Package *package, bool imported) {
    // First let‘s declare all structs for circular dependencies etc.
    for (auto &valueType : package->valueTypes()) {
        if (!valueType->requiresCopyReification()) {
            valueType->createUnspecificReification();
        }
        declarator().declareTypeDefinition(valueType.get(), false);
    }
    for (auto &klass : package->classes()) {
        klass->createUnspecificReification();
        declarator().declareTypeDefinition(klass.get(), true);
    }

    // Now we populate the structs so they are ready for all function declaration stuff to come
    for (auto &valueType : package->valueTypes()) {
        generateTypeDefinition(valueType.get(), false);
    }
    for (auto &klass : package->classes()) {
        generateTypeDefinition(klass.get(), true);
    }

    buildClassObjectBoxInfo();
    buildCallableBoxInfo();

    for (auto &protocol : package->protocols()) {
        createProtocolFunctionTypes(protocol.get());
    }
    for (auto &valueType : package->valueTypes()) {
        valueType->eachReification([&](ValueTypeReification &reification) {
            valueType->eachFunction([this, imported, &reification](Function *function) {
                if (!imported && !function->requiresCopyReification()) {
                    function->createUnspecificReification();
                }
                declarator_->declareLlvmFunction(function, &reification);
            });
            if (valueType->isManaged()) {
                valueType->deinitializer()->createUnspecificReification();
                declarator_->declareLlvmFunction(valueType->deinitializer(), &reification);
                valueType->copyRetain()->createUnspecificReification();
                declarator_->declareLlvmFunction(valueType->copyRetain(), &reification);
            }

            if (!imported) {
                valueType->setBoxInfo(declarator().declareBoxInfo(mangleBoxInfoName(valueType.get(), &reification)));
                valueType->setBoxRetainRelease(buildBoxRetainRelease(Type(valueType.get()), reification));
                protocolsTableGenerator_->generate(Type(valueType.get()), &reification);
                valueType->boxInfo()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
                    protocolsTableGenerator_->createProtocolTable(valueType.get(), reification),
                    valueType->boxRetainRelease().first, valueType->boxRetainRelease().second
                }));
            }
            else {
                protocolsTableGenerator_->declareImported(Type(valueType.get()), &reification);
            }
        });
    }
    for (auto &klass : package->classes()) {
        klass->eachReification([&](ClassReification &reification) {
            VTCreator(klass.get(), *declarator_).build(&reification);
            if (!imported) {
                klass->setBoxRetainRelease(classObjectRetainRelease_);
                protocolsTableGenerator_->generate(Type(klass.get()), &reification);
                createClassInfo(klass.get(), &reification);
            }
            else {
                protocolsTableGenerator_->declareImported(Type(klass.get()), &reification);
                declarator().declareImportedClassInfo(klass.get(), &reification);
            }
        });
    }
    for (auto &function : package->functions()) {
        if (!imported) {
            function->createUnspecificReification();
        }
        declarator_->declareLlvmFunction(function.get(), nullptr);
    }
}

void CodeGenerator::emitModule(const std::string &outPath, bool printIr) {
    llvm::legacy::PassManager pass;

    auto fileType = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code errorCode;
    llvm::raw_fd_ostream dest(outPath, errorCode, llvm::sys::fs::F_None);
    if (targetMachine_->addPassesToEmitFile(pass, dest, fileType)) {
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
        valueType->eachReification([&](TypeDefinitionReification &reification) {
            valueType->eachFunction([&](auto *function) {
                generateFunction(function, &reification);
            });
            if (!imported && valueType->isManaged()) {
                generateFunction(valueType->deinitializer(), &reification);
                generateFunction(valueType->copyRetain(), &reification);
            }
        });
    }
    for (auto &klass : package->classes()) {
        klass->eachReification([&](TypeDefinitionReification &reification) {
            klass->eachFunction([&](auto *function) {
                generateFunction(function, &reification);
            });
            if (!imported) {
                generateFunction(klass->deinitializer(), &reification);
            }
        });
    }
    for (auto &function : package->functions()) {
        generateFunction(function.get(), nullptr);
    }
}

void CodeGenerator::generateFunction(Function *function, const TypeDefinitionReification *reification) {
    if (!function->isExternal()) {
        function->eachReification([this, function, reification](auto &funcReifi) {
            typeHelper_.withReificationContext(ReificationContext(funcReifi.arguments, reification), [&] {
                FunctionCodeGenerator(function, funcReifi.function, this).generate();
            });
            optimizationManager_->optimize(funcReifi.function);
        });
    }
}

void CodeGenerator::createProtocolFunctionTypes(Protocol *protocol) {
    size_t tableIndex = 0;
    for (auto function : protocol->methodList()) {
        function->createUnspecificReification();
        function->eachReification([this, function, &tableIndex](auto &reification) {
            typeHelper_.withReificationContext(ReificationContext(reification.arguments, nullptr), [&] {
                reification.setFunctionType(typeHelper().functionTypeFor(function));
                reification.setVti(tableIndex++);
            });
        });
    }
}

void CodeGenerator::generateTypeDefinition(TypeDefinition *typeDef, bool isClass) {
    typeDef->eachReificationTDR([&](TypeDefinitionReification &reification) {
        if (!isClass && dynamic_cast<ValueType *>(typeDef)->isPrimitive()) {
            return;
        }

        typeHelper_.withReificationContext(ReificationContext({}, &reification), [&] {
            std::vector<llvm::Type *> types;
            if (isClass) {
                types.emplace_back(llvm::Type::getInt8PtrTy(context_));
                types.emplace_back(typeHelper().classInfo()->getPointerTo());
            }

            for (auto &ivar : typeDef->instanceVariables()) {
                types.emplace_back(typeHelper().llvmTypeFor(ivar.type->type()));
            }

            llvm::dyn_cast<llvm::StructType>(reification.type)->setBody(types);
        });
    });
}

void CodeGenerator::createClassInfo(Class *klass, ClassReification *reification) {
    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(context()), klass->virtualTable().size());
    auto virtualTable = new llvm::GlobalVariable(*module(), type, true,
                                                 llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                                 llvm::ConstantArray::get(type, klass->virtualTable()));
    llvm::Constant *superclass;
    if (klass->superclass() != nullptr) {
        superclass = klass->superclass()->unspecificReification().classInfo;
    }
    else {
        superclass = llvm::ConstantPointerNull::get(typeHelper_.classInfo()->getPointerTo());
    }

    auto protocolTable = protocolsTableGenerator_->createProtocolTable(klass, *reification);
    auto gep = llvm::ConstantExpr::getInBoundsGetElementPtr(virtualTable->getType()->getElementType(), virtualTable,
                                                 llvm::ArrayRef<llvm::Constant *>{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(context()), 0)
    });
    auto initializer = llvm::ConstantStruct::get(typeHelper_.classInfo(), { superclass, gep, protocolTable });
    auto info = new llvm::GlobalVariable(*module(), typeHelper_.classInfo(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassInfoName(klass, reification));
    reification->classInfo = info;
}

std::pair<llvm::Function*, llvm::Function*>
CodeGenerator::buildBoxRetainRelease(const Type &type, const TypeDefinitionReification &reification) {
    return buildBoxRetainRelease(type, mangleBoxRelease(type.typeDefinition(), &reification),
                                 mangleBoxRetain(type.typeDefinition(), &reification));
}

std::pair<llvm::Function*, llvm::Function*>
CodeGenerator::buildBoxRetainRelease(const Type &type, std::string retainName, std::string releaseName) {
    auto release = llvm::Function::Create(typeHelper().boxRetainRelease(),
                                          llvm::GlobalValue::LinkageTypes::ExternalLinkage, retainName, module_.get());
    auto retain = llvm::Function::Create(typeHelper().boxRetainRelease(),
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, releaseName, module_.get());

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
    std::tie(retain, release) = classObjectRetainRelease_ = buildBoxRetainRelease(Type(klass), "class.boxRetain",
                                                                                  "class.boxRelease");
    retain->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    release->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);

    declarator().boxInfoForObjects()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
        llvm::ConstantPointerNull::get(typeHelper().protocolConformanceEntry()->getPointerTo()),
        retain, release
    }));
    declarator().boxInfoForObjects()->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
}

void CodeGenerator::buildCallableBoxInfo() {
    llvm::Function *retain, *release;
    std::tie(retain, release) = buildBoxRetainRelease(Type(Type::noReturn(), {}), "callable.boxRetain",
                                                      "callable.boxRelease");
    retain->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    release->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);

    declarator().boxInfoForCallables()->setInitializer(llvm::ConstantStruct::get(typeHelper().boxInfo(), {
        llvm::ConstantPointerNull::get(typeHelper().protocolConformanceEntry()->getPointerTo()),
        retain, release
    }));
    declarator().boxInfoForCallables()->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
}

}  // namespace EmojicodeCompiler
