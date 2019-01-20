//
// Created by Theo Weidmann on 03.02.18.
//

#include "Declarator.hpp"
#include "CodeGenerator.hpp"
#include "Functions/Initializer.hpp"
#include "Generation/ReificationContext.hpp"
#include "LLVMTypeHelper.hpp"
#include "Mangler.hpp"
#include "Package/Package.hpp"
#include "ProtocolsTableGenerator.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "VTCreator.hpp"
#include "Compiler.hpp"
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

namespace EmojicodeCompiler {

Declarator::Declarator(CodeGenerator *generator) : generator_(generator) {
    declareRunTime();
}

void EmojicodeCompiler::Declarator::declareRunTime() {
    alloc_ = declareRunTimeFunction("ejcAlloc", llvm::Type::getInt8PtrTy(generator_->context()),
                                         llvm::Type::getInt64Ty(generator_->context()));
    alloc_->addAttribute(0, llvm::Attribute::NonNull);
    alloc_->addFnAttr(llvm::Attribute::getWithAllocSizeArgs(generator_->context(), 0, llvm::Optional<unsigned>()));

    panic_ = declareRunTimeFunction("ejcPanic", llvm::Type::getVoidTy(generator_->context()),
                                    llvm::Type::getInt8PtrTy(generator_->context()));
    panic_->addFnAttr(llvm::Attribute::NoReturn);
    panic_->addFnAttr(llvm::Attribute::Cold);  // A program should panic rarely.

    inheritsFrom_ = declareRunTimeFunction("ejcInheritsFrom", llvm::Type::getInt1Ty(generator_->context()), {
        generator_->typeHelper().classInfo()->getPointerTo(), generator_->typeHelper().classInfo()->getPointerTo()
    });
    inheritsFrom_->addFnAttr(llvm::Attribute::ReadOnly);
    inheritsFrom_->addParamAttr(0, llvm::Attribute::NonNull);
    inheritsFrom_->addParamAttr(1, llvm::Attribute::NonNull);

    findProtocolConformance_ = declareRunTimeFunction("ejcFindProtocolConformance",
                                                      generator_->typeHelper().protocolConformance()->getPointerTo(), {
        generator_->typeHelper().protocolConformanceEntry()->getPointerTo(), llvm::Type::getInt1PtrTy(generator_->context())
    });
    findProtocolConformance_->addFnAttr(llvm::Attribute::ReadOnly);
    findProtocolConformance_->addParamAttr(0, llvm::Attribute::NonNull);

    boxInfoClassObjects_ = declareBoxInfo("class.boxInfo");
    boxInfoCallables_ = declareBoxInfo("callable.boxInfo");

    retain_ = declareMemoryRunTimeFunction("ejcRetain");
    release_ = declareMemoryRunTimeFunction("ejcRelease");
    releaseMemory_ = declareMemoryRunTimeFunction("ejcReleaseMemory");
    releaseCapture_ = declareMemoryRunTimeFunction("ejcReleaseCapture");

    isOnlyReference_ = declareRunTimeFunction("ejcIsOnlyReference", llvm::Type::getInt1Ty(generator_->context()),
                                     llvm::Type::getInt8PtrTy(generator_->context()));
    isOnlyReference_->addParamAttr(0, llvm::Attribute::NonNull);
    isOnlyReference_->addParamAttr(0, llvm::Attribute::NoCapture);

    ignoreBlock_ = new llvm::GlobalVariable(*generator_->module(), llvm::Type::getInt8Ty(generator_->context()), true,
                                            llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr,
                                            "ejcIgnoreBlock");

    auto compiler = generator_->compiler();
    compiler->sInteger->createUnspecificReification().type = llvm::Type::getInt64Ty(generator_->context());
    compiler->sReal->createUnspecificReification().type = llvm::Type::getDoubleTy(generator_->context());
    compiler->sBoolean->createUnspecificReification().type = llvm::Type::getInt1Ty(generator_->context());
    compiler->sMemory->createUnspecificReification().type = llvm::Type::getInt8PtrTy(generator_->context());
    compiler->sByte->createUnspecificReification().type = llvm::Type::getInt8Ty(generator_->context());
}

llvm::Function* Declarator::declareRunTimeFunction(const char *name, llvm::Type *returnType,
                                                                      llvm::ArrayRef<llvm::Type *> args) {
    auto fn = llvm::Function::Create(llvm::FunctionType::get(returnType, args, false), llvm::Function::ExternalLinkage,
                                     name, generator_->module());
    fn->addFnAttr(llvm::Attribute::NoUnwind);
    fn->addFnAttr(llvm::Attribute::NoRecurse);
    return fn;
}

llvm::Function* Declarator::declareMemoryRunTimeFunction(const char *name) {
    auto fn = declareRunTimeFunction(name, llvm::Type::getVoidTy(generator_->context()),
                                     llvm::Type::getInt8PtrTy(generator_->context()));
    fn->addParamAttr(0, llvm::Attribute::NonNull);
    fn->addParamAttr(0, llvm::Attribute::NoCapture);
    return fn;
}

void Declarator::declareImportedClassInfo(Class *klass, ClassReification *reification) {
    reification->classInfo = new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().classInfo(), true,
                                                      llvm::GlobalValue::ExternalLinkage, nullptr,
                                                      mangleClassInfoName(klass, reification));
}

llvm::Function::LinkageTypes Declarator::linkageForFunction(Function *function) const {
    if (function->isInline() && function->package()->isImported()) {
        return llvm::Function::AvailableExternallyLinkage;
    }
    if ((function->accessLevel() == AccessLevel::Private && !function->isExternal() &&
         (function->owner() == nullptr || !function->owner()->exported())) || function->isClosure()) {
        return llvm::Function::PrivateLinkage;
    }
    return llvm::Function::ExternalLinkage;
}

void Declarator::declareTypeDefinition(TypeDefinition *typeDef, bool isClass) {
    typeDef->eachReificationTDR([&](TypeDefinitionReification &reification) {
        if (reification.type != nullptr) {
            return;
        }
        else if (!isClass && dynamic_cast<ValueType*>(typeDef)->isPrimitive()) {
            reification.type = llvm::Type::getInt64Ty(generator_->context());
            return;
        }

        auto structType = llvm::StructType::create(generator_->context(), mangleTypeDefinition(typeDef, &reification));
        reification.type = structType;
    });
}

llvm::Function* Declarator::createLlvmFunction(Function *function, ReificationContext reificationContext) const {
    llvm::FunctionType *ft;
    generator_->typeHelper().withReificationContext(reificationContext, [&] {
        ft = generator_->typeHelper().functionTypeFor(function);
    });
    auto name = function->externalName().empty() ? mangleFunction(function, reificationContext)
                : function->externalName();

    auto fn = llvm::Function::Create(ft, linkageForFunction(function), name, generator_->module());
    fn->addFnAttr(llvm::Attribute::NoUnwind);
    if (function->isInline()) {
        fn->addFnAttr(llvm::Attribute::InlineHint);
    }

    size_t i = function->isClosure() ? 1 : 0;
    if (hasThisArgument(function) && !function->isClosure()) {
        addParamDereferenceable(function->typeContext().calleeType(), i, fn, false);
        if (function->functionType() == FunctionType::ObjectInitializer) {
            if (!dynamic_cast<Initializer*>(function)->errorProne()) {
                fn->addParamAttr(i, llvm::Attribute::Returned);
                addParamDereferenceable(function->typeContext().calleeType(), 0, fn, true);
            }
        }
        else if (!function->memoryFlowTypeForThis().isEscaping()) {
            fn->addParamAttr(i, llvm::Attribute::NoCapture);
        }

        if (function->typeContext().calleeType().type() == TypeType::ValueType && !function->mutating()) {
            fn->addParamAttr(0, llvm::Attribute::ReadOnly);
        }

        i++;
    }
    else if (function->functionType() == FunctionType::ObjectInitializer
             && !dynamic_cast<Initializer*>(function)->errorProne()) {  // foreign initializers
        addParamDereferenceable(function->typeContext().calleeType(), 0, fn, true);
    }
    for (auto &param : function->parameters()) {
        addParamAttrs(param, i, fn);
        i++;
    }
    addParamDereferenceable(function->returnType()->type(), 0, fn, true);
    return fn;
}

void Declarator::declareLlvmFunction(Function *function, const TypeDefinitionReification *reification) const {
    if (function->externalName() == "ejcBuiltIn") {
        return;
    }
    function->eachReification([this, function, reification](FunctionReification &funcReifi) {
        funcReifi.function = createLlvmFunction(function, ReificationContext(funcReifi.arguments, reification));
    });
}

void Declarator::addParamAttrs(const Parameter &param, size_t index, llvm::Function *function) const {
    if (!param.memoryFlowType.isEscaping() && param.type->type().type() == TypeType::Class) {
        function->addParamAttr(index, llvm::Attribute::NoCapture);
    }

    addParamDereferenceable(param.type->type(), index, function, false);
}

void Declarator::addParamDereferenceable(const Type &type, size_t index, llvm::Function *function, bool ret) const {
    if (generator_->typeHelper().isDereferenceable(type)) {
        auto llvmType = ret ? function->getFunctionType()->getReturnType() :
                              function->getFunctionType()->getParamType(index);
        auto elementType = llvm::dyn_cast<llvm::PointerType>(llvmType)->getElementType();
        if (ret) {
            function->addDereferenceableAttr(0, generator_->querySize(elementType));
        }
        else {
            function->addParamAttrs(index, llvm::AttrBuilder().addDereferenceableAttr(generator_->querySize(elementType)));
        }
    }
}

llvm::GlobalVariable* Declarator::declareBoxInfo(const std::string &name) {
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().boxInfo(), true,
                                    llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, name);
}

}  // namespace EmojicodeCompiler
