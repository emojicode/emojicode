//
//  TypeDescriptionGenerator.cpp
//  runtime
//
//  Created by Theo Weidmann on 25.03.19.
//

#include "TypeDescriptionGenerator.hpp"
#include "Generation/LLVMTypeHelper.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/Protocol.hpp"
#include "Compiler.hpp"
#include "Generation/RunTimeHelper.hpp"

namespace EmojicodeCompiler {

void TypeDescriptionGenerator::addType(const Type &type) {
    llvm::Constant *genericInfo;
    auto notype = type.unoptionalized().unboxed();
    switch (notype.type()) {
        case TypeType::Class:
            genericInfo = buildConstant00Gep(fg_->typeHelper().classInfo(), notype.klass()->classInfo(), fg_->ctx());
            break;
        case TypeType::Protocol:
            genericInfo = notype.protocol()->rtti();
            break;
        case TypeType::ValueType:
        case TypeType::Enum:
            genericInfo = buildConstant00Gep(fg_->typeHelper().boxInfo(), fg_->boxInfoFor(notype), fg_->ctx());
            break;
        case TypeType::Something:
            genericInfo = fg_->generator()->runTime().somethingRtti();
            break;
        case TypeType::Someobject:
            genericInfo = fg_->generator()->runTime().someobjectRtti();
            break;
        case TypeType::GenericVariable:
            addDynamic(fg_->builder().CreateLoad(fg_->genericArgsPtr()), notype.genericVariableIndex());
            return;
        case TypeType::LocalGenericVariable:
            addDynamic(fg_->functionGenericArgs(), notype.genericVariableIndex());
            return;
        case TypeType::Callable:
        case TypeType::TypeAsValue:
        case TypeType::MultiProtocol:
            genericInfo = fg_->generator()->runTime().someobjectRtti();
            fg_->compiler()->warn(SourcePosition(), "Run-time type information for multiprotocols, callables and type "\
                                  "values is not available yet. Casts and other reflection may not behave as "\
                                  "expected with these types.");
            break;
        case TypeType::NoReturn:
        case TypeType::Box:
        case TypeType::Optional:
        case TypeType::StorageExpectation:
            throw std::logic_error("Cannot create type description for compile-time type.");
    }

    auto strct = llvm::ConstantStruct::get(fg_->typeHelper().typeDescription(), {
        genericInfo,
        type.type() == TypeType::Optional ? llvm::ConstantInt::getTrue(fg_->ctx()) : llvm::ConstantInt::getFalse(fg_->ctx())
    });
    types_.emplace_back(strct);

    if (!notype.canHaveGenericArguments()) return;
    for (auto &arg : notype.genericArguments()) {
        addType(arg);
    }
}

void TypeDescriptionGenerator::addDynamic(llvm::Value *gargs, size_t index) {
    dynamic_++;
    auto idf = fg_->generator()->runTime().indexTypeDescription();
    auto td = index > 0 ? fg_->builder().CreateCall(idf, { gargs, fg_->int64(index) }) : gargs;
    auto size = fg_->builder().CreateCall(fg_->generator()->runTime().typeDescriptionLength(), td);
    types_.emplace_back(td, size);
}

llvm::Value* TypeDescriptionGenerator::generate(const std::vector<Type> &types) {
    assert(types_.empty());
    for (auto &type : types) {
        addType(type);
    }
    return finish();
}

llvm::Value* TypeDescriptionGenerator::generate(const std::vector<std::shared_ptr<ASTType>> &types) {
    assert(types_.empty());
    for (auto &type : types) {
        addType(type->type());
    }
    return finish();
}

llvm::Value* TypeDescriptionGenerator::generate(const Type &type) {
    assert(types_.empty());
    addType(type);
    return finish();
}

llvm::Value* TypeDescriptionGenerator::finish() {
    if (dynamic_ == 0) return finishStatic();

    auto typeDesc = fg_->typeHelper().typeDescription();
    llvm::Value *size = fg_->int64(types_.size() - dynamic_);
    for (auto &tdv : types_) {
        if (tdv.isCopy()) {
            size = fg_->builder().CreateAdd(size, tdv.size);
        }
    }

    llvm::Value *array = fg_->builder().CreateBitCast(fg_->builder().CreateCall(fg_->generator()->runTime().alloc(),
                                                                                fg_->builder().CreateMul(fg_->sizeOf(typeDesc), size), "alloc"), typeDesc->getPointerTo());
    auto current = array;

    for (auto &tdv : types_) {
        if (tdv.isCopy()) {
            fg_->builder().CreateMemCpy(current, 0, tdv.from, 0, fg_->builder().CreateMul(fg_->sizeOf(typeDesc), tdv.size));
            current = fg_->builder().CreateInBoundsGEP(current, tdv.size);
        }
        else {
            fg_->builder().CreateStore(tdv.concrete, current);
            current = fg_->builder().CreateConstInBoundsGEP1_32(typeDesc, current, 1);
        }
    }
    return array;
}

llvm::Value* TypeDescriptionGenerator::finishStatic() {
    auto typeDesc = fg_->typeHelper().typeDescription();
    auto type = llvm::ArrayType::get(typeDesc, types_.size());
    std::vector<llvm::Constant*> cargs;
    for (auto &arg : types_) {
        cargs.emplace_back(arg.concrete);
    }
    auto init = llvm::ConstantArray::get(type, cargs);
    auto var = new llvm::GlobalVariable(*fg_->generator()->module(), type, true,
                                        llvm::GlobalValue::LinkageTypes::PrivateLinkage, init);
    var->setUnnamedAddr(llvm::GlobalVariable::UnnamedAddr::Global);
    return buildConstant00Gep(type, var, fg_->ctx());
}

}  // namespace EmojicodeCompiler
