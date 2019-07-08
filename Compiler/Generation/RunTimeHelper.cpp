//
// Created by Theo Weidmann on 03.02.18.
//

#include "RunTimeHelper.hpp"
#include "CodeGenerator.hpp"
#include "LLVMTypeHelper.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include "Compiler.hpp"
#include "BoxRetainReleaseBuilder.hpp"
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

namespace EmojicodeCompiler {

RunTimeHelper::RunTimeHelper(CodeGenerator *generator) : generator_(generator) {}

void RunTimeHelper::declareRunTime() {
    alloc_ = declareRunTimeFunction("ejcAlloc", llvm::Type::getInt8PtrTy(generator_->context()),
                                    llvm::Type::getInt64Ty(generator_->context()));
    alloc_->addAttribute(0, llvm::Attribute::NonNull);
    alloc_->addFnAttr(llvm::Attribute::getWithAllocSizeArgs(generator_->context(), 0, llvm::Optional<unsigned>()));
    alloc_->addAttribute(llvm::AttributeList::ReturnIndex, llvm::Attribute::NoAlias);

    panic_ = declareRunTimeFunction("ejcPanic", llvm::Type::getVoidTy(generator_->context()),
                                    llvm::Type::getInt8PtrTy(generator_->context()));
    panic_->addFnAttr(llvm::Attribute::NoReturn);
    panic_->addFnAttr(llvm::Attribute::Cold);  // A program should panic rarely.

    inheritsFrom_ = declareRunTimeFunction("ejcInheritsFrom", llvm::Type::getInt1Ty(generator_->context()), {
        generator_->typeHelper().classInfo()->getPointerTo(), generator_->typeHelper().classInfo()->getPointerTo()
    });
    inheritsFrom_->addFnAttr(llvm::Attribute::ReadOnly);
    inheritsFrom_->addFnAttr(llvm::Attribute::Speculatable);
    inheritsFrom_->addParamAttr(0, llvm::Attribute::NonNull);
    inheritsFrom_->addParamAttr(1, llvm::Attribute::NonNull);

    findProtocolConformance_ = declareRunTimeFunction("ejcFindProtocolConformance",
                                                      generator_->typeHelper().protocolConformance()->getPointerTo(), {
        generator_->typeHelper().protocolConformanceEntry()->getPointerTo(),
        generator_->typeHelper().runTimeTypeInfo()->getPointerTo()
    });
    findProtocolConformance_->addFnAttr(llvm::Attribute::ReadOnly);
    findProtocolConformance_->addFnAttr(llvm::Attribute::Speculatable);
    findProtocolConformance_->addParamAttr(0, llvm::Attribute::NonNull);
    findProtocolConformance_->addParamAttr(1, llvm::Attribute::NonNull);

    checkGenericArgs_ = declareRunTimeFunction("ejcCheckGenericArgs", llvm::Type::getInt1Ty(generator_->context()), {
        generator_->typeHelper().typeDescription()->getPointerTo(),
        generator_->typeHelper().typeDescription()->getPointerTo(),
        llvm::Type::getInt16Ty(generator_->context()),
        llvm::Type::getInt16Ty(generator_->context())
    });
    checkGenericArgs_->removeFnAttr(llvm::Attribute::NoRecurse);
    checkGenericArgs_->addFnAttr(llvm::Attribute::ReadOnly);
    checkGenericArgs_->addFnAttr(llvm::Attribute::Speculatable);
    checkGenericArgs_->addParamAttr(0, llvm::Attribute::NonNull);
    checkGenericArgs_->addParamAttr(1, llvm::Attribute::NonNull);

    typeDescriptionLength_ = declareRunTimeFunction("ejcTypeDescriptionLength",
                                                   llvm::Type::getInt64Ty(generator_->context()),
                                                   generator_->typeHelper().typeDescription()->getPointerTo());
    typeDescriptionLength_->addFnAttr(llvm::Attribute::ReadOnly);
    typeDescriptionLength_->addFnAttr(llvm::Attribute::Speculatable);
    typeDescriptionLength_->addParamAttr(0, llvm::Attribute::NonNull);

    indexTypeDescription_ = declareRunTimeFunction("ejcIndexTypeDescription",
                                                    generator_->typeHelper().typeDescription()->getPointerTo(),
                                                    { generator_->typeHelper().typeDescription()->getPointerTo(),
                                                      llvm::Type::getInt64Ty(generator_->context()) });
    indexTypeDescription_->addFnAttr(llvm::Attribute::ReadOnly);
    indexTypeDescription_->addFnAttr(llvm::Attribute::Speculatable);
    indexTypeDescription_->addParamAttr(0, llvm::Attribute::NonNull);

    retain_ = declareMemoryRunTimeFunction("ejcRetain");
    retainMemory_ = declareMemoryRunTimeFunction("ejcRetainMemory");
    releaseMemory_ = declareMemoryRunTimeFunction("ejcReleaseMemory");
    retain_->addFnAttr(llvm::Attribute::InaccessibleMemOrArgMemOnly);
    retainMemory_->addFnAttr(llvm::Attribute::InaccessibleMemOrArgMemOnly);
    releaseMemory_->addFnAttr(llvm::Attribute::InaccessibleMemOrArgMemOnly);

    /// All of these call deinitializers and we cannot make any predictions about their memory usage
    release_ = declareMemoryRunTimeFunction("ejcRelease");
    releaseWithoutDeinit_ = declareMemoryRunTimeFunction("ejcReleaseWithoutDeinit");
    releaseCapture_ = declareMemoryRunTimeFunction("ejcReleaseCapture");
    releaseLocal_ = declareMemoryRunTimeFunction("ejcReleaseLocal");

    isOnlyReference_ = declareRunTimeFunction("ejcIsOnlyReference", llvm::Type::getInt1Ty(generator_->context()),
                                     llvm::Type::getInt8PtrTy(generator_->context()));
    isOnlyReference_->addParamAttr(0, llvm::Attribute::NonNull);
    isOnlyReference_->addParamAttr(0, llvm::Attribute::NoCapture);

    ignoreBlock_ = new llvm::GlobalVariable(*generator_->module(), llvm::Type::getInt8Ty(generator_->context()), true,
                                            llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr,
                                            "ejcIgnoreBlock");

    somethingRTTI_ = createAbstractRtti("something_rtti");
    someobjectRTTI_ = createAbstractRtti("someobject_rtti");

    // This has to be last as buildRetainRelease uses functions declared above!
    boxInfoClassObjects_ = declareBoxInfo("class.boxInfo");
    classObjectRetainRelease_ = buildRetainRelease(Type(generator_->compiler()->sString), "class.boxRetain",
                                                   "class.boxRelease", boxInfoClassObjects_);
    boxInfoCallables_ = declareBoxInfo("callable.boxInfo");
    buildRetainRelease(Type(Type::noReturn(), {}, Type::noReturn()), "callable.boxRetain", "callable.boxRelease",
                       boxInfoCallables_);

    malloc_ = declareRunTimeFunction("malloc", llvm::Type::getInt8PtrTy(generator_->context()),
                                    llvm::Type::getInt64Ty(generator_->context()));
    malloc_->removeFnAttr(llvm::Attribute::NoRecurse);
    malloc_->addAttribute(llvm::AttributeList::ReturnIndex, llvm::Attribute::NonNull);
    malloc_->addFnAttr(llvm::Attribute::getWithAllocSizeArgs(generator_->context(), 0, llvm::Optional<unsigned>()));
    malloc_->addAttribute(llvm::AttributeList::ReturnIndex, llvm::Attribute::NoAlias);

    free_ = declareRunTimeFunction("free", llvm::Type::getVoidTy(generator_->context()),
                                   llvm::Type::getInt8PtrTy(generator_->context()));
    free_->removeFnAttr(llvm::Attribute::NoRecurse);
    free_->addParamAttr(0, llvm::Attribute::NonNull);
}

llvm::Function* RunTimeHelper::declareRunTimeFunction(const char *name, llvm::Type *returnType,
                                                                      llvm::ArrayRef<llvm::Type *> args) {
    auto fn = llvm::Function::Create(llvm::FunctionType::get(returnType, args, false), llvm::Function::ExternalLinkage,
                                     name, generator_->module());
    fn->addFnAttr(llvm::Attribute::NoUnwind);
    fn->addFnAttr(llvm::Attribute::NoRecurse);
    return fn;
}

llvm::Function* RunTimeHelper::declareMemoryRunTimeFunction(const char *name) {
    auto fn = declareRunTimeFunction(name, llvm::Type::getVoidTy(generator_->context()),
                                     llvm::Type::getInt8PtrTy(generator_->context()));
    fn->addParamAttr(0, llvm::Attribute::NonNull);
    fn->addParamAttr(0, llvm::Attribute::NoCapture);
    return fn;
}

llvm::GlobalVariable* RunTimeHelper::declareBoxInfo(const std::string &name) {
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().boxInfo(), true,
                                    llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, name);
}

llvm::GlobalVariable* RunTimeHelper::createAbstractRtti(const char *name) {
    auto init = llvm::ConstantStruct::get(generator_->typeHelper().runTimeTypeInfo(),
                                          llvm::ConstantInt::get(llvm::Type::getInt16Ty(generator_->context()), 0),
                                          llvm::ConstantInt::get(llvm::Type::getInt16Ty(generator_->context()), 0),
                                          llvm::ConstantInt::get(llvm::Type::getInt8Ty(generator_->context()),
                                                                 RunTimeTypeInfoFlags::Abstract));
    return new llvm::GlobalVariable(*generator_->module(), generator_->typeHelper().runTimeTypeInfo(), true,
                                    llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage, init, name);
}

std::pair<llvm::Function*, llvm::Function*> RunTimeHelper::buildRetainRelease(const Type &prototype,
                                                                              const char *retainName,
                                                                              const char *releaseName,
                                                                              llvm::GlobalVariable *boxInfo) {
    llvm::Function *retain, *release;
    std::tie(retain, release) = buildBoxRetainRelease(generator_, prototype);
    retain->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    release->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    retain->setName(retainName);
    release->setName(releaseName);

    boxInfo->setInitializer(llvm::ConstantStruct::get(generator_->typeHelper().boxInfo(), {
        llvm::ConstantAggregateZero::get(generator_->typeHelper().runTimeTypeInfo()),
        retain, release,
        llvm::ConstantPointerNull::get(generator_->typeHelper().protocolConformanceEntry()->getPointerTo())
    }));
    boxInfo->setLinkage(llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    return {retain, release};
}

llvm::Constant* RunTimeHelper::createRtti(TypeDefinition *generic, RunTimeTypeInfoFlags::Flags flag) {
    return llvm::ConstantStruct::get(generator_->typeHelper().runTimeTypeInfo(), {
        llvm::ConstantInt::get(llvm::Type::getInt16Ty(generator_->context()), generic->genericParameters().size()),
        llvm::ConstantInt::get(llvm::Type::getInt16Ty(generator_->context()), generic->offset()),
        llvm::ConstantInt::get(llvm::Type::getInt8Ty(generator_->context()), flag),
    });
}

}  // namespace EmojicodeCompiler
