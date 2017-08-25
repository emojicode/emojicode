//
//  ASTExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Types/Enum.hpp"
#include "../Application.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

void ASTExpr::generate(FnCodeGenerator *fncg) const {
    if (temporarilyScoped_) {
        fncg->scoper().pushScope();
    }
    generateExpr(fncg);
    if (temporarilyScoped_) {
        fncg->scoper().popScope(fncg->wr().count());
    }
}

Type ASTGetVariable::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto var = analyser->scoper().getVariable(name_, position());
    copyVariableAstInfo(var, analyser);
    return var.variable.type();
}

void ASTGetVariable::generateExpr(FnCodeGenerator *fncg) const {
    auto &var = inInstanceScope_ ? fncg->instanceScoper().getVariable(varId_) : fncg->scoper().getVariable(varId_);
    if (reference_) {
        fncg->pushVariableReference(var.stackIndex, inInstanceScope_);
        return;
    }

    fncg->pushVariable(var.stackIndex, inInstanceScope_, var.type);
}

Type ASTMetaTypeInstantiation::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    analyser->validateMetability(type_, position());
    type_.setMeta(true);
    return type_;
}

void ASTMetaTypeInstantiation::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
    fncg->wr().writeInstruction(type_.eclass()->index);
}

Type ASTCast::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    Type originalType = value_->analyse(analyser, expectation);
    if (originalType.compatibleTo(type, analyser->typeContext())) {
        analyser->app()->error(CompilerError(position(), "Unnecessary cast."));
    }
    else if (!type.compatibleTo(originalType, analyser->typeContext())) {
        auto typeString = type.toString(analyser->typeContext());
        analyser->app()->error(CompilerError(position(), "Cast to unrelated type ", typeString," will always fail."));
    }

    if (type.type() == TypeType::Class) {
        if (!type.genericArguments().empty()) {
            analyser->app()->error(CompilerError(position(), "Class casts with generic arguments are not available."));
        }

        if (originalType.type() == TypeType::Someobject || originalType.type() == TypeType::Class) {
            if (originalType.optional()) {
                throw CompilerError(position(), "Downcast on classes with optionals not possible.");
            }
            castType_ = CastType::ClassDowncast;
            assert(originalType.storageType() == StorageType::Simple && originalType.size() == 1);
        }
        else {
            castType_ = CastType::ToClass;
            assert(originalType.storageType() == StorageType::Box);
        }
    }
    else if (type.type() == TypeType::Protocol && isStatic(typeExpr_->availability())) {
        if (!type.genericArguments().empty()) {
            analyser->app()->error(CompilerError(position(), "Cannot cast to generic protocols."));
        }
        castType_ = CastType::ToProtocol;
        assert(originalType.storageType() == StorageType::Box);
    }
    else if ((type.type() == TypeType::ValueType || type.type() == TypeType::Enum)
             && isStatic(typeExpr_->availability())) {
        castType_ = CastType::ToValueType;
        assert(originalType.storageType() == StorageType::Box);
        type.forceBox();
    }
    else {
        auto typeString = type.toString(analyser->typeContext());
        throw CompilerError(position(), "You cannot cast to ", typeString, ".");
    }

    type.setOptional(true);
    return type;
}

void ASTCast::generateExpr(FnCodeGenerator *fncg) const {
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
}

Type ASTConditionalAssignment::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (!t.optional()) {
        throw CompilerError(position(), "Condition assignment can only be used with optionals.");
    }

    t.setReference(false);
    t.setOptional(false);

    auto &variable = analyser->scoper().currentScope().declareVariable(varName_, t, true, position());
    variable.initialize();
    varId_ = variable.id();

    return Type::boolean();
}

void ASTConditionalAssignment::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
    auto &var = fncg->scoper().declareVariable(varId_, expr_->expressionType());
    fncg->copyToVariable(var.stackIndex, false, expr_->expressionType());
    fncg->pushVariableReference(var.stackIndex, false);
    fncg->wr().writeInstruction({ INS_IS_NOTHINGNESS, INS_INVERT_BOOLEAN });
    var.stackIndex.increment();
    var.type.setOptional(false);
}

Type ASTTypeMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(callee_, expectation);

    if (type.optional()) {
        analyser->app()->warn(position(), "You cannot call optionals on 🍬.");
    }

    Function *method;
    if (type.type() == TypeType::Class) {
        method = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), position());
    }
    else if ((type.type() == TypeType::ValueType || type.type() == TypeType::Enum)
             && isStatic(callee_->availability())) {
        method = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), position());
        valueType_ = true;
    }
    else {
        throw CompilerError(position(), "You can’t call type methods on ", type.toString(analyser->typeContext()), ".");
    }
    return analyser->analyseFunctionCall(&args_, type, method);
}

void ASTTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
    auto ins = valueType_ ? INS_CALL_FUNCTION : INS_DISPATCH_TYPE_METHOD;
    TypeMethodCallCodeGenerator(fncg, ins).generate(*callee_, args_, name_);
}

Type ASTSuperMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (analyser->function()->functionType() != FunctionType::ObjectMethod) {
        throw CompilerError(position(), "Not within an object-context.");
    }

    Class *superclass = analyser->typeContext().calleeType().eclass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(position(), "Class has no superclass.");
    }

    Function *method = superclass->getMethod(name_, Type(superclass, false), analyser->typeContext(), position());
    calleeType_ = Type(superclass, true);
    return analyser->analyseFunctionCall(&args_, calleeType_, method);
}

void ASTSuperMethod::generateExpr(FnCodeGenerator *fncg) const {
    SuperCallCodeGenerator(fncg, INS_DISPATCH_SUPER).generate(calleeType_, args_, name_);
}

Type ASTCallableCall::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false, false), &callable_);
    if (type.type() != TypeType::Callable) {
        throw CompilerError(position(), "Given value is not callable.");
    }
    for (size_t i = 1; i < type.genericArguments().size(); i++) {
        analyser->expectType(type.genericArguments()[i], &args_.arguments()[i - 1]);
    }
    return type.genericArguments()[0];
}

void ASTCallableCall::generateExpr(FnCodeGenerator *fncg) const {
    CallableCallCodeGenerator(fncg).generate(*callable_, args_, std::u32string());
}

Type ASTCaptureMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false, false), &callee_);

    analyser->validateMethodCapturability(type, position());
    auto function = type.typeDefinition()->getMethod(name_, type, analyser->typeContext(), position());
    function->deprecatedWarning(position());
    return function->type();
}

void ASTCaptureMethod::generateExpr(FnCodeGenerator *fncg) const {
    callee_->generate(fncg);
    fncg->wr().writeInstruction(INS_CAPTURE_METHOD);
    fncg->wr().writeInstruction(callee_->expressionType().eclass()->lookupMethod(name_)->vtiForUse());
}

Type ASTCaptureTypeMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(callee_, expectation);
    analyser->validateMethodCapturability(type, position());
    auto function = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), position());
    function->deprecatedWarning(position());
    contextedFunction_ = type.type() == TypeType::ValueType || type.type() == TypeType::Enum;
    return function->type();
}

void ASTCaptureTypeMethod::generateExpr(FnCodeGenerator *fncg) const {
    if (contextedFunction_) {
        fncg->wr().writeInstruction(INS_CAPTURE_CONTEXTED_FUNCTION);
    }
    else {
        callee_->generate(fncg);
        fncg->wr().writeInstruction(INS_CAPTURE_TYPE_METHOD);
    }
    fncg->wr().writeInstruction(callee_->expressionType().typeDefinition()->lookupTypeMethod(name_)->vtiForUse());
}

}  // namespace EmojicodeCompiler
