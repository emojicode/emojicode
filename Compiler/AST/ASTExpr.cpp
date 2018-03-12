//
//  ASTExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTExpr.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTMetaTypeInstantiation::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    analyser->validateMetability(type_, position());
    return Type(MakeTypeAsValue, type_);
}

Type ASTSizeOf::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    return analyser->integer();
}

Type ASTCast::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    Type originalType = value_->analyse(analyser, expectation);
    if (originalType.compatibleTo(type, analyser->typeContext())) {
        analyser->compiler()->error(CompilerError(position(), "Unnecessary cast."));
    }
    else if (!type.compatibleTo(originalType, analyser->typeContext())) {
        auto typeString = type.toString(analyser->typeContext());
        analyser->compiler()->error(CompilerError(position(), "Cast to unrelated type ", typeString," will always fail."));
    }

    if (type.type() == TypeType::Class) {
        if (!type.genericArguments().empty()) {
            analyser->compiler()->error(CompilerError(position(), "Class casts with generic arguments are not available."));
        }

        if (originalType.type() == TypeType::Someobject || originalType.type() == TypeType::Class) {
            castType_ = CastType::ClassDowncast;
            assert(originalType.storageType() == StorageType::Simple);
        }
        else {
            castType_ = CastType::ToClass;
            assert(originalType.storageType() == StorageType::Box);
        }
    }
    else if (type.type() == TypeType::Protocol && isStatic(typeExpr_->availability())) {
        if (!type.genericArguments().empty()) {
            analyser->compiler()->error(CompilerError(position(), "Cannot cast to generic protocols."));
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

    return Type(MakeOptional, type);
}

Type ASTConditionalAssignment::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type t = analyser->expect(TypeExpectation(false, false), &expr_);
    if (t.type() != TypeType::Optional) {
        throw CompilerError(position(), "Condition assignment can only be used with optionals.");
    }

    t = t.optionalType();

    auto &variable = analyser->scoper().currentScope().declareVariable(varName_, t, true, position());
    variable.initialize();
    varId_ = variable.id();

    return analyser->boolean();
}

Type ASTTypeMethod::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(callee_, expectation);

    Function *method;
    if (type.type() == TypeType::Class) {
        method = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), args_.isImperative(),
                                                      position());
    }
    else if ((type.type() == TypeType::ValueType || type.type() == TypeType::Enum)
             && isStatic(callee_->availability())) {
        method = type.typeDefinition()->getTypeMethod(name_, type, analyser->typeContext(), args_.isImperative(),
                                                      position());
        valueType_ = true;
    }
    else {
        throw CompilerError(position(), "You can’t call type methods on ", type.toString(analyser->typeContext()), ".");
    }
    return analyser->analyseFunctionCall(&args_, type, method);
}

Type ASTSuper::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    if (analyser->function()->functionType() == FunctionType::ObjectInitializer) {
        analyseSuperInit(analyser);
        return Type::noReturn();
    }
    if (analyser->function()->functionType() != FunctionType::ObjectMethod) {
        throw CompilerError(position(), "Not within an object-context.");
    }

    Class *superclass = analyser->typeContext().calleeType().eclass()->superclass();

    if (superclass == nullptr) {
        throw CompilerError(position(), "Class has no superclass.");
    }

    Function *method = superclass->getMethod(name_, Type(superclass), analyser->typeContext(),
                                             args_.isImperative(), position());
    calleeType_ = Type(superclass);
    return analyser->analyseFunctionCall(&args_, calleeType_, method);
}

void ASTSuper::analyseSuperInit(FunctionAnalyser *analyser) {
    if (!isSuperconstructorRequired(analyser->function()->functionType())) {
        throw CompilerError(position(), "🐐 can only be used inside initializers.");
    }
    if (analyser->typeContext().calleeType().eclass()->superclass() == nullptr) {
        throw CompilerError(position(), "🐐 can only be used if the class inherits from another.");
    }
    if (analyser->pathAnalyser().hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
        analyser->compiler()->error(CompilerError(position(), "Superinitializer might have already been called."));
    }

    analyser->scoper().instanceScope()->unintializedVariablesCheck(position(), "Instance variable \"", "\" must be "
            "initialized before calling the superinitializer.");

    init_ = true;
    Class *eclass = analyser->typeContext().calleeType().eclass();
    auto initializer = eclass->superclass()->getInitializer(name_, Type(eclass),
                                                            analyser->typeContext(), position());
    calleeType_ = Type(eclass->superclass());
    analyser->analyseFunctionCall(&args_, calleeType_, initializer);

    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::CalledSuperInitializer);
}

Type ASTCallableCall::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    Type type = analyser->expect(TypeExpectation(false, false, false), &callable_);
    if (type.type() != TypeType::Callable) {
        throw CompilerError(position(), "Given value is not callable.");
    }
    for (size_t i = 1; i < type.genericArguments().size(); i++) {
        analyser->expectType(type.genericArguments()[i], &args_.parameters()[i - 1]);
    }
    return type.genericArguments()[0];
}

}  // namespace EmojicodeCompiler
