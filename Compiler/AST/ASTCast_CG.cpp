//
//  ASTCast.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 25.09.18.
//

#include "ASTCast.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "Generation/Declarator.hpp"

namespace EmojicodeCompiler {

Value* ASTCast::generate(FunctionCodeGenerator *fg) const {
    if (castType_ == CastType::ClassDowncast) {
        return downcast(fg);
    }

    auto box = fg->createEntryAlloca(fg->typeHelper().box());
    fg->builder().CreateStore(expr_->generate(fg), box);
    Value *is = nullptr;
    switch (castType_) {
        case CastType::ToClass:
            is = castToClass(fg, box);
            break;
        case CastType::ToValueType:
            is = castToValueType(fg, box);
            break;
        case CastType::ToProtocol:
            return castToProtocol(fg, box);
        case CastType::ClassDowncast:
            throw std::logic_error("unreachable");
    }

    fg->createIfElse(is, []() {}, [fg, box]() {
        fg->buildMakeNoValue(box);
    });
    return fg->builder().CreateLoad(box);
}

Value* ASTCast::downcast(FunctionCodeGenerator *fg) const {
    auto value = expr_->generate(fg);
    auto info = fg->buildGetClassInfoFromObject(value);
    auto toType = typeExpr_->expressionType();
    auto inheritsFrom = fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                                 { info, typeExpr_->generate(fg) });
    return fg->createIfElsePhi(inheritsFrom, [toType, fg, value]() {
        auto casted = fg->builder().CreateBitCast(value, fg->typeHelper().llvmTypeFor(toType));
        return fg->buildSimpleOptionalWithValue(casted, toType.optionalized());
    }, [fg, toType]() {
        return fg->buildSimpleOptionalWithoutValue(toType.optionalized());
    });
}

Value* ASTCast::boxInfo(FunctionCodeGenerator *fg, Value *box) const {
    if (expr_->expressionType().boxedFor().type() == TypeType::Protocol) {
        auto protocolConPtr = fg->builder().CreateBitCast(fg->builder().CreateLoad(fg->buildGetBoxInfoPtr(box)),
                                                          fg->typeHelper().protocolConformance()->getPointerTo());
        return fg->builder().CreateLoad(fg->builder().CreateConstInBoundsGEP2_32(fg->typeHelper().protocolConformance(), protocolConPtr, 0, 2));
    }
    return fg->builder().CreateLoad(fg->buildGetBoxInfoPtr(box));
}

Value* ASTCast::castToValueType(FunctionCodeGenerator *fg, Value *box) const {
    return fg->builder().CreateICmpEQ(boxInfo(fg, box), typeExpr_->generate(fg));
}

Value* ASTCast::castToClass(FunctionCodeGenerator *fg, Value *box) const {
    auto toType = typeExpr_->expressionType();
    auto isExpBoxInfo = fg->builder().CreateICmpEQ(boxInfo(fg, box), fg->boxInfoFor(toType));

    return fg->createIfElsePhi(isExpBoxInfo, [&] {
        auto obj = fg->builder().CreateLoad(fg->buildGetBoxValuePtr(box, typeExpr_->expressionType()));
        return fg->builder().CreateCall(fg->generator()->declarator().inheritsFrom(),
                                        { fg->buildGetClassInfoFromObject(obj), typeExpr_->generate(fg) });
    }, [fg] {
        return llvm::ConstantInt::getFalse(fg->generator()->context());
    });
}

Value* ASTCast::castToProtocol(FunctionCodeGenerator *fg, Value *box) const {
    auto conformance = fg->buildFindProtocolConformance(box, boxInfo(fg, box), typeExpr_->generate(fg));

    auto confPtrTy = fg->typeHelper().protocolConformance()->getPointerTo();
    auto confNullptr = llvm::ConstantPointerNull::get(confPtrTy);

    fg->createIfElse(fg->builder().CreateICmpNE(conformance, confNullptr), [&] {
        auto infoPtr = fg->buildGetBoxInfoPtr(box);
        fg->builder().CreateStore(conformance, fg->builder().CreateBitCast(infoPtr, confPtrTy->getPointerTo()));
    }, [&] {
        fg->buildMakeNoValue(box);
    });
    return fg->builder().CreateLoad(box);
}

}  // namespace EmojicodeCompiler
