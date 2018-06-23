//
// Created by Theo Weidmann on 03.02.18.
//

#include "Declarator.hpp"
#include "Functions/Initializer.hpp"
#include "Generation/ReificationContext.hpp"
#include "LLVMTypeHelper.hpp"
#include "Mangler.hpp"
#include "Package/Package.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>

namespace EmojicodeCompiler {

Declarator::Declarator(llvm::LLVMContext &context, llvm::Module &module, LLVMTypeHelper &typeHelper)
        : context_(context), module_(module), typeHelper_(typeHelper) {
    declareRunTime();
}

void EmojicodeCompiler::Declarator::declareRunTime() {
    runTimeNew_ = declareRunTimeFunction("ejcAlloc", llvm::Type::getInt8PtrTy(context_),
                                         llvm::Type::getInt64Ty(context_));
    runTimeNew_->addFnAttr(llvm::Attribute::NoUnwind);

    panic_ = declareRunTimeFunction("ejcPanic", llvm::Type::getVoidTy(context_), llvm::Type::getInt8PtrTy(context_));
    panic_->addFnAttr(llvm::Attribute::NoUnwind);
    panic_->addFnAttr(llvm::Attribute::NoReturn);
    panic_->addFnAttr(llvm::Attribute::Cold);  // A program should panic rarely.

    classValueTypeMeta_ = new llvm::GlobalVariable(module_, typeHelper_.valueTypeMeta(), true,
                                                   llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                   nullptr, "ejcRunTimeClassValueTypeMeta");
}

llvm::Function* Declarator::declareRunTimeFunction(const char *name, llvm::Type *returnType,
                                                                      llvm::ArrayRef<llvm::Type *> args) {
    auto ft = llvm::FunctionType::get(returnType, args, false);
    return llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, &module_);
}

void Declarator::declareImportedClassMeta(Class *klass) {
    auto meta = new llvm::GlobalVariable(module_, typeHelper_.classMeta(), true,
                                         llvm::GlobalValue::ExternalLinkage, nullptr,
                                         mangleClassMetaName(klass));
    klass->setClassMeta(meta);
}

void Declarator::declareImportedPackageSymbols(Package *package) {
    auto ptg = ProtocolsTableGenerator(context_, module_, typeHelper_);
    for (auto &valueType : package->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
        ptg.declareImportedProtocolsTable(Type(valueType.get()));
    }
    for (auto &protocol : package->protocols()) {
        size_t tableIndex = 0;
        for (auto function : protocol->methodList()) {
            function->createUnspecificReification();
            function->eachReification([this, function, &tableIndex](auto &reification) {
                auto context = ReificationContext(*function, reification);
                typeHelper_.setReificationContext(&context);
                reification.entity.setFunctionType(typeHelper_.functionTypeFor(function));
                reification.entity.setVti(tableIndex++);
                typeHelper_.setReificationContext(nullptr);
            });
        }
    }
    for (auto &klass : package->classes()) {
        if (klass->hasSubclass()) {
            VTCreator(klass.get(), *this).build();
        }
        else {
            VTCreator(klass.get(), *this).assign();
        }
        ptg.declareImportedProtocolsTable(Type(klass.get()));
        declareImportedClassMeta(klass.get());
    }
    for (auto &function : package->functions()) {
        declareLlvmFunction(function.get());
    }
}

void Declarator::declareLlvmFunction(Function *function) const {
    function->eachReification([this, function](auto &reification) {
        auto context = ReificationContext(*function, reification);
        typeHelper_.setReificationContext(&context);
        auto ft = typeHelper_.functionTypeFor(function);
        typeHelper_.setReificationContext(nullptr);
        auto name = function->externalName().empty() ? mangleFunction(function, reification.arguments)
                                                     : function->externalName();
        auto linkage = function->accessLevel() == AccessLevel::Private && !function->isExternal()
                                                                       ? llvm::Function::PrivateLinkage
                                                                       : llvm::Function::ExternalLinkage;
        reification.entity.function = llvm::Function::Create(ft, linkage, name, &module_);
        reification.entity.function->addFnAttr(llvm::Attribute::NoUnwind);

        if (function->typeContext().calleeType().type() == TypeType::ValueType && !function->mutating()) {
            reification.entity.function->addParamAttr(0, llvm::Attribute::ReadOnly);
        }
    });
}

}  // namespace EmojicodeCompiler
