//
//  ASTCast.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#include "ASTCast.hpp"
#include "Generation/Declarator.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Types/ValueType.hpp"
#include "Types/Class.hpp"
#include "Generation/RunTimeTypeInfoFlags.hpp"
#include "Types/TypeContext.hpp"

namespace EmojicodeCompiler {

Value* ASTCast::generate(FunctionCodeGenerator *fg) const {
    if (isDowncast_) {
        return downcast(fg);
    }

    auto box = expr_->generate(fg);
    return fg->builder().CreateCall(getCastFunction(fg->generator()),
                                    { typeExpr_->generate(fg), box, boxInfo(fg, box) });
}

Value* getRtti(FunctionCodeGenerator *fg, Value *typeDescPtr) {
    return fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().typeDescription(),
                                                                             typeDescPtr, 0, 0), "rtti");
}

Value* ASTCast::downcast(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto info = fg->buildGetClassInfoFromObject(value);
    auto toType = typeExpr_->expressionType();
    auto classInfo = fg->builder().CreateBitCast(getRtti(fg, typeExpr_->generate(fg)),
                                                 fg->typeHelper().classInfo()->getPointerTo());
    auto inheritsFrom = fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                                 { info, classInfo });
    return fg->createIfElsePhi(inheritsFrom, [&] {
        auto casted = fg->builder().CreateBitCast(value, fg->typeHelper().llvmTypeFor(toType));
        return fg->buildSimpleOptionalWithValue(casted, toType.optionalized());
    }, [&] {
        fg->release(value, expr_->expressionType());
        return fg->buildSimpleOptionalWithoutValue(toType.optionalized());
    });
}

Value* ASTCast::boxInfo(FunctionCodeGenerator *fg, Value *box) const {
    if (expr_->expressionType().boxedFor().type() == TypeType::Protocol) {
        auto protocolConPtr = fg->builder().CreateBitCast(fg->builder().CreateLoad(fg->buildGetBoxInfoPtr(box)),
                                                          fg->typeHelper().protocolConformance()->getPointerTo());
        return fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().protocolConformance(),
                                                                                 protocolConPtr, 0, 2));
    }
    return fg->builder().CreateLoad(fg->buildGetBoxInfoPtr(box));
}

llvm::Function* ASTCast::kFunction = nullptr;

llvm::Function* ASTCast::getCastFunction(CodeGenerator *cg) {
    if (kFunction != nullptr) return kFunction;

    auto ft = llvm::FunctionType::get(cg->typeHelper().box(), { cg->typeHelper().typeDescription()->getPointerTo(),
        cg->typeHelper().box()->getPointerTo(), cg->typeHelper().boxInfo()->getPointerTo() }, false);
    kFunction = llvm::Function::Create(ft, llvm::GlobalValue::LinkageTypes::LinkOnceAnyLinkage, "dynamicCast",
                                       cg->module());
    kFunction->addFnAttr(llvm::Attribute::AlwaysInline);
    kFunction->addFnAttr(llvm::Attribute::ReadOnly);
    kFunction->addFnAttr(llvm::Attribute::NoRecurse);
    kFunction->addFnAttr(llvm::Attribute::NoUnwind);
    FunctionCodeGenerator fg(kFunction, cg, std::make_unique<TypeContext>());
    fg.createEntry();

    auto it = kFunction->args().begin();
    llvm::Argument* typeDescription = &*(it++), *box = &*(it++), *boxInfo = &*it;

    auto rtti = getRtti(&fg, typeDescription);
    auto flag = fg.builder().CreateLoad(fg.builder().CreateConstInBoundsGEP2_32(fg.typeHelper().runTimeTypeInfo(),
                                                                                rtti, 0, 2));

    auto valueType = fg.createBlock("valueType"), protocol = fg.createBlock("protocol"),
        klass = fg.createBlock("class"), finish = fg.createBlock("finishCast");
    auto swtch = fg.builder().CreateSwitch(flag, valueType, 3);
    swtch->addCase(fg.int8(RunTimeTypeInfoFlags::Class), klass);
    swtch->addCase(fg.int8(RunTimeTypeInfoFlags::Protocol), protocol);

    fg.builder().SetInsertPoint(klass);
    auto classBox = castToClass(&fg, box, typeDescription, boxInfo, rtti);
    auto classIncoming = fg.builder().GetInsertBlock();
    fg.builder().CreateBr(finish);

    fg.builder().SetInsertPoint(valueType);
    auto vtBox = castToValueType(&fg, box, typeDescription, flag, boxInfo, rtti);
    auto vtIncoming = fg.builder().GetInsertBlock();
    fg.builder().CreateBr(finish);

    fg.builder().SetInsertPoint(protocol);
    auto protocolBox = castToProtocol(&fg, box, rtti, boxInfo);
    auto protocolIncoming = fg.builder().GetInsertBlock();
    fg.builder().CreateBr(finish);

    fg.builder().SetInsertPoint(finish);
    auto phi = fg.builder().CreatePHI(fg.typeHelper().box(), 3);
    phi->addIncoming(classBox, classIncoming);
    phi->addIncoming(vtBox, vtIncoming);
    phi->addIncoming(protocolBox, protocolIncoming);
    fg.builder().CreateRet(phi);
    return kFunction;
}

llvm::Value* checkGeneric(FunctionCodeGenerator *fg, llvm::Value *mainCheck, llvm::Value *genericArgs, int argsOffset,
                          llvm::Value *typeDescription, llvm::Value *box, llvm::Value *rtti) {
    return fg->createIfElsePhi(mainCheck, [&]{
        auto rttiType = fg->typeHelper().runTimeTypeInfo();
        auto ownGeneric = fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(rttiType, rtti, 0, 0));
        return fg->createIfElsePhi(fg->builder().CreateIsNotNull(ownGeneric), [&] {
            auto genericsOk = fg->builder().CreateCall(fg->generator()->declarator().checkGenericArgs(), {
                genericArgs,
                fg->builder().CreateConstInBoundsGEP1_32(fg->typeHelper().typeDescription(), typeDescription, 1),
                ownGeneric,
                fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(rttiType, rtti, 0, 1))
            });
            return fg->builder().CreateSelect(genericsOk, fg->builder().CreateLoad(box),
                                              fg->buildBoxWithoutValue());
        }, [&] { return fg->builder().CreateLoad(box); });
    }, [&] {
        return fg->buildBoxWithoutValue();
    });
}

Value* ASTCast::castToValueType(FunctionCodeGenerator *fg, Value *box, Value *typeDescription, Value *flag,
                                Value *boxInfo, llvm::Value *rtti) {
    auto bi = fg->builder().CreateICmpEQ(boxInfo,
                                         fg->builder().CreateBitCast(rtti, fg->typeHelper().boxInfo()->getPointerTo()));

    auto boxPtr = fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().box(), box, 0, 1);
    auto isLocalValueType = fg->builder().CreateICmpEQ(flag, fg->int8(RunTimeTypeInfoFlags::ValueType));
    auto genericArgs = fg->createIfElsePhi(isLocalValueType, [&] {
        // first element in generic value type must be generic arguments
        return fg->builder().CreateBitCast(boxPtr, fg->typeHelper().typeDescription()->getPointerTo()->getPointerTo());
    }, [&] {
        // remote value types have one level of indirection
        auto ptrType = fg->typeHelper().typeDescription()->getPointerTo()->getPointerTo()->getPointerTo();
        auto rptr = fg->builder().CreateBitCast(boxPtr, ptrType);
        return fg->builder().CreateLoad(rptr);
    });
    return checkGeneric(fg, bi, fg->builder().CreateLoad(genericArgs), 0, typeDescription, box, rtti);
}

Value* ASTCast::castToClass(FunctionCodeGenerator *fg, Value *box, Value *typeDescription,
                            Value *boxInfo, llvm::Value *rtti) {
    auto isExpBoxInfo = fg->builder().CreateICmpEQ(boxInfo, fg->generator()->declarator().boxInfoForObjects());
    auto strct = llvm::StructType::get(llvm::Type::getInt8PtrTy(fg->ctx()),
                                       fg->typeHelper().classInfo()->getPointerTo(),
                                       fg->typeHelper().typeDescription()->getPointerTo());

    return fg->createIfElsePhi(isExpBoxInfo, [&]() -> llvm::Value* {
        auto obj = fg->builder().CreateLoad(fg->buildGetBoxValuePtr(box, strct->getPointerTo()->getPointerTo()));
        auto ci = fg->builder().CreateBitCast(rtti, fg->typeHelper().classInfo()->getPointerTo());
        auto inherits = fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                                 { fg->buildGetClassInfoFromObject(obj), ci });
        auto genericArgs = fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(strct, obj, 0, 2));
        return checkGeneric(fg, inherits, genericArgs, 2, typeDescription, box, rtti);
    }, [fg] {
        return fg->buildBoxWithoutValue();
    });
}

Value* ASTCast::castToProtocol(FunctionCodeGenerator *fg, Value *box, Value *rtti, Value *boxInfo) {
    auto conformance = fg->buildFindProtocolConformance(box, boxInfo, rtti);
    auto confPtrTy = fg->typeHelper().protocolConformance()->getPointerTo();
    return fg->createIfElsePhi(fg->builder().CreateIsNotNull(conformance), [&] {
        auto boxCopy = fg->createEntryAlloca(fg->typeHelper().box());
        fg->builder().CreateStore(fg->builder().CreateLoad(box), boxCopy);
        auto infoPtr = fg->buildGetBoxInfoPtr(boxCopy);
        fg->builder().CreateStore(conformance, fg->builder().CreateBitCast(infoPtr, confPtrTy->getPointerTo()));
        return fg->builder().CreateLoad(boxCopy);
    }, [&] {
        return fg->buildBoxWithoutValue();
    });
}

}  // namespace EmojicodeCompiler
