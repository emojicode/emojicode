//
//  ASTExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Application.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Class.hpp"

namespace EmojicodeCompiler {

Type ASTMetaTypeInstantiation::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    analyser->validateMetability(type_, position());
    type_.setMeta(true);
    return type_;
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

Type ASTSuperMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (analyser->function()->functionType() != FunctionType::ObjectMethod) {
        throw CompilerError(position(), "Not within an object-context.");
    }

    Class *superclass = analyser->typeContext().calleeType().eclass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(position(), "Class has no superclass.");
    }

    Function *method = superclass->getMethod(name_, Type(superclass, false), analyser->typeContext(), position());
    calleeType_ = Type(superclass, false);
    return analyser->analyseFunctionCall(&args_, calleeType_, method);
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

Type ASTCaptureMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false, false), &callee_);

    analyser->validateMethodCapturability(type, position());
    auto function = type.typeDefinition()->getMethod(name_, type, analyser->typeContext(), position());
    function->deprecatedWarning(position());
    return function->type();
}

Type ASTCaptureTypeMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(callee_, expectation);
    analyser->validateMethodCapturability(type, position());
    auto function = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), position());
    function->deprecatedWarning(position());
    contextedFunction_ = type.type() == TypeType::ValueType || type.type() == TypeType::Enum;
    return function->type();
}

}  // namespace EmojicodeCompiler
