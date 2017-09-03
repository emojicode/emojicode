//
//  ASTExpr_CG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "ASTProxyExpr.hpp"
#include "ASTTypeExpr.hpp"

namespace EmojicodeCompiler {

Value* ASTExpr::generate(FnCodeGenerator *fncg) const {
    if (temporarilyScoped_) {
        fncg->scoper().pushScope();
    }
    auto value = generateExpr(fncg);
    if (temporarilyScoped_) {
        fncg->scoper().popScope(fncg->wr().count());
    }
    return value;
}

Value* ASTGetVariable::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = inInstanceScope_ ? fncg->instanceScoper().getVariable(varId_) : fncg->scoper().getVariable(varId_);
    if (reference_) {
        fncg->pushVariableReference(var.stackIndex, inInstanceScope_);
        return nullptr;
    }

    fncg->pushVariable(var.stackIndex, inInstanceScope_, var.type);
    return nullptr;
}


Value* ASTMetaTypeInstantiation::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
    fncg->wr().writeInstruction(type_.eclass()->index);
    return nullptr;
}

Value* ASTCast::generateExpr(FnCodeGenerator *fncg) const {
    typeExpr_->generate(fncg);
    value_->generate(fncg);
    switch (castType_) {
        case CastType::ClassDowncast:
            fncg->wr().writeInstruction(INS_DOWNCAST_TO_CLASS);
            break;
        case CastType::ToClass:
            fncg->wr().writeInstruction(INS_CAST_TO_CLASS);
            break;
        case CastType::ToValueType:
            fncg->wr().writeInstruction(INS_CAST_TO_VALUE_TYPE);
            fncg->wr().writeInstruction(typeExpr_->expressionType().boxIdentifier());
            break;
        case CastType::ToProtocol:
            fncg->wr().writeInstruction(INS_CAST_TO_PROTOCOL);
            fncg->wr().writeInstruction(typeExpr_->expressionType().protocol()->index);
            break;
    }
    return nullptr;
}

Value* ASTConditionalAssignment::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    auto &var = fncg->scoper().declareVariable(varId_, expr_->expressionType());
    fncg->copyToVariable(var.stackIndex, false, expr_->expressionType());
    fncg->pushVariableReference(var.stackIndex, false);
    fncg->wr().writeInstruction({ INS_IS_NOTHINGNESS, INS_INVERT_BOOLEAN });
    var.stackIndex.increment();
    var.type.setOptional(false);
    return nullptr;
}

Value* ASTTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
    auto ins = valueType_ ? INS_CALL_FUNCTION : INS_DISPATCH_TYPE_METHOD;
    TypeMethodCallCodeGenerator(fncg, ins).generate(*callee_, callee_->expressionType(), args_, name_);
    return nullptr;
}

Value* ASTSuperMethod::generateExpr(FnCodeGenerator *fncg) const {
    SuperCallCodeGenerator(fncg, INS_DISPATCH_SUPER).generate(calleeType_, args_, name_);
    return nullptr;
}


Value* ASTCallableCall::generateExpr(FnCodeGenerator *fncg) const {
    CallableCallCodeGenerator(fncg).generate(*callable_, callable_->expressionType(), args_, std::u32string());
    return nullptr;
}

Value* ASTCaptureMethod::generateExpr(FnCodeGenerator *fncg) const {
    callee_->generate(fncg);
    fncg->wr().writeInstruction(INS_CAPTURE_METHOD);
    fncg->wr().writeInstruction(callee_->expressionType().eclass()->lookupMethod(name_)->vtiForUse());
    return nullptr;
}

Value* ASTCaptureTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
    if (contextedFunction_) {
        fncg->wr().writeInstruction(INS_CAPTURE_CONTEXTED_FUNCTION);
    }
    else {
        callee_->generate(fncg);
        fncg->wr().writeInstruction(INS_CAPTURE_TYPE_METHOD);
    }
    fncg->wr().writeInstruction(callee_->expressionType().typeDefinition()->lookupTypeMethod(name_)->vtiForUse());
    return nullptr;
}

}  // namespace EmojicodeCompiler
