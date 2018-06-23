//
//  ASTMethod.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "ASTVariables.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Emojis.h"

namespace EmojicodeCompiler {

Type ASTMethodable::analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee) {
    Type otype = callee->analyse(analyser, TypeExpectation());
    return analyseMethodCall(analyser, name, callee, otype);

}

Type ASTMethodable::analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee, const Type &otype) {
    determineCalleeType(analyser, name, callee, otype);

    if (calleeType_.unboxedType() == TypeType::MultiProtocol) {
        return analyseMultiProtocolCall(analyser, name);
    }
    if (calleeType_.type() == TypeType::TypeAsValue) {
        return analyseTypeMethodCall(analyser, name, callee);
    }

    determineCallType(analyser);

    method_ = calleeType_.typeDefinition()->getMethod(name, calleeType_, analyser->typeContext(),
                                                      args_.isImperative(), position());

    if (calleeType_.type() == TypeType::Class && (method_->accessLevel() == AccessLevel::Private || calleeType_.isExact())) {
        callType_ = CallType::StaticDispatch;
    }

    checkMutation(analyser, callee, otype);
    return analyser->analyseFunctionCall(&args_, calleeType_, method_);
}

void ASTMethodable::determineCalleeType(FunctionAnalyser *analyser, const std::u32string &name,
                                        std::shared_ptr<ASTExpr> &callee, const Type &otype) {
    Type type = otype.resolveOnSuperArgumentsAndConstraints(analyser->typeContext());
    if (builtIn(analyser, type, name)) {
        calleeType_ = analyser->comply(type, TypeExpectation(false, false), &callee);
    }
    else {
        calleeType_ = analyser->comply(type, TypeExpectation(true, false), &callee).resolveOnSuperArgumentsAndConstraints(analyser->typeContext());
    }
}

void ASTMethodable::determineCallType(const FunctionAnalyser *analyser) {
    if (calleeType_.type() == TypeType::ValueType) {
        callType_ = CallType::StaticDispatch;
    }
    else if (calleeType_.unboxedType() == TypeType::Protocol) {
        callType_ = CallType::DynamicProtocolDispatch;
    }
    else if (calleeType_.type() == TypeType::Enum) {
        callType_ = CallType::StaticDispatch;
    }
    else if (calleeType_.type() == TypeType::Class) {
        callType_ = CallType::DynamicDispatch;
    }
    else {
        throw CompilerError(position(), calleeType_.toString(analyser->typeContext()), " does not provide methods.");
    }
}

void ASTMethodable::checkMutation(FunctionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee,
                                  const Type &type) const {
    if (type.type() == TypeType::ValueType && method_->mutating()) {
        if (!type.isMutable()) {
            analyser->compiler()->error(CompilerError(position(), utf8(method_->name()),
                                                      " was marked 🖍 but callee is not mutable."));
        }
        else if (auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(callee)) {
            analyser->scoper().currentScope().getLocalVariable(varNode->name()).mutate(position());
        }
    }
}

Type ASTMethodable::analyseTypeMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                                          std::shared_ptr<ASTExpr> &callee) {
    calleeType_ = calleeType_.typeOfTypeValue();

    if (calleeType_.type() == TypeType::ValueType || calleeType_.type() == TypeType::Enum) {
        callType_ = CallType::StaticContextfreeDispatch;
    }
    else if (calleeType_.type() == TypeType::Class) {
        callType_ = CallType::DynamicDispatchOnType;
    }
    else {
        throw CompilerError(position(), calleeType_.toString(analyser->typeContext()), " does not provide methods.");
    }

    method_ = calleeType_.typeDefinition()->getTypeMethod(name, calleeType_, analyser->typeContext(),
                                                              args_.isImperative(), position());

    if (calleeType_.type() == TypeType::Class && (method_->accessLevel() == AccessLevel::Private || calleeType_.isExact())) {
        callType_ = CallType::StaticDispatch;
    }

    return analyser->analyseFunctionCall(&args_, calleeType_, method_);
}

Type ASTMethodable::analyseMultiProtocolCall(FunctionAnalyser *analyser, const std::u32string &name) {
    for (; multiprotocolN_ < calleeType_.protocols().size(); multiprotocolN_++) {
        auto &protocol = calleeType_.protocols()[multiprotocolN_];
        if ((method_ = protocol.protocol()->lookupMethod(name, args_.isImperative())) != nullptr) {
            builtIn_ = BuiltInType::Multiprotocol;
            callType_ = CallType::DynamicProtocolDispatch;
            return analyser->analyseFunctionCall(&args_, protocol, method_);
        }
    }
    throw CompilerError(position(), "No type in ", calleeType_.toString(analyser->typeContext()),
                        " provides a method ", utf8(name), ".");
}

bool ASTMethodable::builtIn(FunctionAnalyser *analyser, const Type &type, const std::u32string &name) {
    if (type.type() != TypeType::ValueType) {
        return false;
    }

    if (type.typeDefinition() == analyser->compiler()->sBoolean) {
        if (name.front() == E_NEGATIVE_SQUARED_CROSS_MARK) {
            builtIn_ = BuiltInType::BooleanNegate;
            return true;
        }
    }
    else if (type.typeDefinition() == analyser->compiler()->sInteger) {
        if (name.front() == E_HUNDRED_POINTS_SYMBOL) {
            builtIn_ = BuiltInType::IntegerToDouble;
            return true;
        }
        if (name.front() == E_NO_ENTRY_SIGN) {
            builtIn_ = BuiltInType::IntegerNot;
            return true;
        }
    }
    else if (type.typeDefinition() == analyser->compiler()->sByte) {
        if (name.front() == E_NO_ENTRY_SIGN) {
            builtIn_ = BuiltInType::IntegerNot;
            return true;
        }
    }
    else if (type.typeDefinition() == analyser->compiler()->sMemory) {
        if (name.front() == 0x1F43D) {
            builtIn_ = BuiltInType::Load;
            return true;
        }
        if (name.front() == 0x1F437) {
            builtIn_ = BuiltInType::Store;
            return true;
        }
    }
    return false;
}

Type ASTMethod::analyse(FunctionAnalyser *analyser, const TypeExpectation &expectation) {
    return analyseMethodCall(analyser, name_, callee_);
}

}  // namespace EmojicodeCompiler
