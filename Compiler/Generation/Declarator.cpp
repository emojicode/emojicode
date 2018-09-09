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
#include "CodeGenerator.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>

namespace EmojicodeCompiler {

Declarator::Declarator(CodeGenerator *generator) : generator_(generator) {
    declareRunTime();
}

void EmojicodeCompiler::Declarator::declareRunTime() {
    alloc_ = declareRunTimeFunction("ejcAlloc", llvm::Type::getInt8PtrTy(generator_->context()),
                                         llvm::Type::getInt64Ty(generator_->context()));

    panic_ = declareRunTimeFunction("ejcPanic", llvm::Type::getVoidTy(generator_->context()),
                                    llvm::Type::getInt8PtrTy(generator_->context()));
    panic_->addFnAttr(llvm::Attribute::NoReturn);
    panic_->addFnAttr(llvm::Attribute::Cold);  // A program should panic rarely.

    inheritsFrom_ = declareRunTimeFunction("ejcInheritsFrom", llvm::Type::getInt1Ty(generator_->context()), {
        generator_->typeHelper().classInfo()->getPointerTo(), generator_->typeHelper().classInfo()->getPointerTo()
    });
    findProtocolConformance_ = declareRunTimeFunction("ejcFindProtocolConformance",
                                                      generator_->typeHelper().protocolConformance()->getPointerTo(), {
        generator_->typeHelper().protocolConformanceEntry()->getPointerTo(), llvm::Type::getInt1PtrTy(generator_->context())
    });

    boxInfoClassObjects_ = declareBoxInfo("class.boxInfo");

    retain_ = declareRunTimeFunction("ejcRetain", llvm::Type::getVoidTy(generator_->context()),
                                     llvm::Type::getInt8PtrTy(generator_->context()));
    release_ = declareRunTimeFunction("ejcRelease", llvm::Type::getVoidTy(generator_->context()),
                                      llvm::Type::getInt8PtrTy(generator_->context()));
    releaseMemory_ = declareRunTimeFunction("ejcReleaseMemory", llvm::Type::getVoidTy(generator_->context()),
                                            llvm::Type::getInt8PtrTy(generator_->context()));

    ignoreBlock_ = new llvm::GlobalVariable(*generator_->module(), llvm::Type::getInt8Ty(generator_->context()), true,
                                            llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr,
                                            "ejcIgnoreBlock");
}

llvm::Function* Declarator::declareRunTimeFunction(const char *name, llvm::Type *returnType,
                                                                      llvm::ArrayRef<llvm::Type *> args) {
    auto fn = llvm::Function::Create(llvm::FunctionType::get(returnType, args, false), llvm::Function::ExternalLinkage,
                                     name, generator_->module());
    fn->addFnAttr(llvm::Attribute::NoUnwind);
    fn->addFnAttr(llvm::Attribute::NoRecurse);
    return fn;
}

void Declarator::declareImportedClassInfo(Class *klass) {
    auto info = new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().classInfo(), true,
                                         llvm::GlobalValue::ExternalLinkage, nullptr,
                                         mangleClassInfoName(klass));
    klass->setClassInfo(info);
}

void Declarator::declareImportedPackageSymbols(Package *package) {
    auto ptg = ProtocolsTableGenerator(generator_);
    for (auto &valueType : package->valueTypes()) {
        valueType->eachFunction([this](auto *function) {
            declareLlvmFunction(function);
        });
        if (valueType->isManaged()) {
            valueType->deinitializer()->createUnspecificReification();
            declareLlvmFunction(valueType->deinitializer());
            valueType->copyRetain()->createUnspecificReification();
            declareLlvmFunction(valueType->copyRetain());
        }
        ptg.declareImported(Type(valueType.get()));
    }
    for (auto &protocol : package->protocols()) {
        size_t tableIndex = 0;
        for (auto function : protocol->methodList()) {
            function->createUnspecificReification();
            function->eachReification([this, function, &tableIndex](auto &reification) {
                auto context = ReificationContext(*function, reification);
                generator_->typeHelper().setReificationContext(&context);
                reification.entity.setFunctionType(generator_->typeHelper().functionTypeFor(function));
                reification.entity.setVti(tableIndex++);
                generator_->typeHelper().setReificationContext(nullptr);
            });
        }
    }
    for (auto &klass : package->classes()) {
        VTCreator(klass.get(), *this).build();
        ptg.declareImported(Type(klass.get()));
        declareImportedClassInfo(klass.get());
    }
    for (auto &function : package->functions()) {
        declareLlvmFunction(function.get());
    }
}

void Declarator::declareLlvmFunction(Function *function) const {
    function->eachReification([this, function](auto &reification) {
        auto context = ReificationContext(*function, reification);
        generator_->typeHelper().setReificationContext(&context);
        auto ft = generator_->typeHelper().functionTypeFor(function);
        generator_->typeHelper().setReificationContext(nullptr);
        auto name = function->externalName().empty() ? mangleFunction(function, reification.arguments)
                                                     : function->externalName();
        auto linkage = function->accessLevel() == AccessLevel::Private && !function->isExternal()
                                                                       ? llvm::Function::PrivateLinkage
                                                                       : llvm::Function::ExternalLinkage;
        reification.entity.function = llvm::Function::Create(ft, linkage, name, generator_->module());
        reification.entity.function->addFnAttr(llvm::Attribute::NoUnwind);

        size_t i = 0;
        if (hasThisArgument(function->functionType())) {
            if (function->memoryFlowTypeForThis() == MFFlowCategory::Borrowing) {
                reification.entity.function->addParamAttr(i, llvm::Attribute::NoCapture);
            }
            i++;
        }
        for (auto &param : function->parameters()) {
            if (param.memoryFlowType == MFFlowCategory::Borrowing && param.type->type().type() == TypeType::Class) {
                reification.entity.function->addParamAttr(i, llvm::Attribute::NoCapture);
            }
            i++;
        }

        if (function->typeContext().calleeType().type() == TypeType::ValueType && !function->mutating()) {
            reification.entity.function->addParamAttr(0, llvm::Attribute::ReadOnly);
        }
    });
}

llvm::GlobalVariable* Declarator::declareBoxInfo(const std::string &name) {
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().boxInfo(), true,
                                    llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, name);
}

}  // namespace EmojicodeCompiler
