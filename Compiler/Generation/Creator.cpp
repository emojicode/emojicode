//
//  PackageCreator.cpp
//  runtime
//
//  Created by Theo Weidmann on 30.03.19.
//

#include "Creator.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include "LLVMTypeHelper.hpp"
#include "ReificationContext.hpp"
#include "Mangler.hpp"
#include "CodeGenerator.hpp"
#include "Package/Package.hpp"
#include "RunTimeHelper.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "BoxRetainReleaseBuilder.hpp"
#include "Compiler.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Constants.h>

namespace EmojicodeCompiler {

void PackageCreator::generate() {
    for (auto &protocol : package_->protocols()) {
        createProtocol(protocol.get());
    }
    for (auto &valueType : package_->valueTypes()) {
        createValueType(valueType.get());
    }
    for (auto &klass : package_->classes()) {
        createClass(klass.get());
    }
    for (auto &function : package_->functions()) {
        createFunction(function.get());
    }
}

void PackageCreator::createProtocol(Protocol *protocol) {
    protocol->setRtti(new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().runTimeTypeInfo(), true,
                                               llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage,
                                               generator_->runTime().createRtti(protocol, RunTimeTypeInfoFlags::Protocol),
                                               mangleProtocolRunTimeTypeInfo(protocol)));

    size_t tableIndex = 0;
    for (auto function : protocol->methodList()) {
        function->createUnspecificReification();
        function->eachReification([&](auto &reification) {
            generator_->typeHelper().withReificationContext(ReificationContext(*function, reification), [&] {
                reification.entity.setFunctionType(generator_->typeHelper().functionTypeFor(function));
                reification.entity.setVti(tableIndex++);
            });
        });
    }
}

void PackageCreator::createClass(Class *klass) {
    klass->createUnspecificReification();
    VTCreator(klass, generator_).build();
    klass->setBoxRetainRelease(generator_->runTime().classObjectRetainRelease());
    createProtocolTables(Type(klass));
    klass->setDestructor(createMemoryFunction(mangleDestructor(klass->type()), generator_, klass));
    createDestructor(klass);
    createClassInfo(klass);
}

void PackageCreator::createDestructor(Class *klass) {
    buildDestructor(generator_, klass);
}

void ImportedPackageCreator::createDestructor(Class *klass) {}

void PackageCreator::createClassInfo(Class *klass) {
    auto type = llvm::ArrayType::get(llvm::Type::getInt8PtrTy(generator_->context()), klass->virtualTable().size());
    auto virtualTable = new llvm::GlobalVariable(*generator_->module(), type, true,
                                                 llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                                                 llvm::ConstantArray::get(type, klass->virtualTable()));
    llvm::Constant *superclass;
    if (klass->superclass() != nullptr) {
        superclass = klass->superclass()->classInfo();
    }
    else {
        superclass = llvm::ConstantPointerNull::get(generator_->typeHelper().classInfo()->getPointerTo());
    }

    auto protocolTable = ProtocolsTableGenerator(generator_).createProtocolTable(klass);
    auto gep = buildConstant00Gep(virtualTable->getType()->getElementType(), virtualTable, generator_->context());
    auto rtti = generator_->runTime().createRtti(klass, RunTimeTypeInfoFlags::Class);
    auto initializer = llvm::ConstantStruct::get(generator_->typeHelper().classInfo(), {
        rtti, gep, protocolTable, superclass,
        llvm::ConstantExpr::getBitCast(klass->destructor(), llvm::Type::getInt8PtrTy(generator_->context())) });
    auto info = new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().classInfo(), true,
                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, initializer,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

void ImportedPackageCreator::createProtocolTables(const Type &type) {
    ProtocolsTableGenerator(generator_).declareImported(type);
}

void PackageCreator::createProtocolTables(const Type &type) {
    ProtocolsTableGenerator(generator_).generate(type);
}

void ImportedPackageCreator::createClassInfo(Class *klass) {
    auto info = new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().classInfo(), true,
                                         llvm::GlobalValue::ExternalLinkage, nullptr,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

void PackageCreator::createValueType(ValueType *valueType) {
    valueType->createUnspecificReification();
    valueType->eachFunction([&](Function *function) { createFunction(function); });

    if (valueType == package_->compiler()->sWeak) {
        valueType->setDestructor(createMemoryFunction("ejcReleaseWeak", generator_, valueType));
        valueType->setCopyRetain(createMemoryFunction("ejcRetainWeak", generator_, valueType));
    }
    else if (valueType->isManaged()) {
        valueType->setDestructor(createMemoryFunction(mangleDestructor(valueType->type()), generator_, valueType));
        valueType->setCopyRetain(createMemoryFunction(mangleCopyRetain(valueType->type()), generator_, valueType));
        createDestructorRetain(valueType);
    }

    createBoxInfo(valueType);
}

void PackageCreator::createDestructorRetain(ValueType *valueType) {
    buildDestructor(generator_, valueType);
    buildCopyRetain(generator_, valueType);
}

void ImportedPackageCreator::createDestructorRetain(ValueType *valueType) {}

void PackageCreator::createBoxInfo(ValueType *valueType) {
    valueType->setBoxInfo(generator_->runTime().declareBoxInfo(mangleBoxInfoName(Type(valueType))));
    valueType->setBoxRetainRelease(buildBoxRetainRelease(generator_, Type(valueType)));
    createProtocolTables(Type(valueType));
    valueType->boxInfo()->setInitializer(llvm::ConstantStruct::get(generator_->typeHelper().boxInfo(), {
        generator_->runTime().createRtti(valueType, generator_->typeHelper().isRemote(Type(valueType)) ?
                                            RunTimeTypeInfoFlags::ValueTypeRemote : RunTimeTypeInfoFlags::ValueType),
        valueType->boxRetainRelease().first, valueType->boxRetainRelease().second,
        ProtocolsTableGenerator(generator_).createProtocolTable(valueType)
    }));
}

void ImportedPackageCreator::createBoxInfo(ValueType *valueType) {
    createProtocolTables(Type(valueType));
}

void PackageCreator::createFunction(Function *function) {
    if (!function->requiresCopyReification()) {
        function->createUnspecificReification();
    }
    generator_->declareLlvmFunction(function);
}

void ImportedPackageCreator::createFunction(Function *function) {
    generator_->declareLlvmFunction(function);
}

}
