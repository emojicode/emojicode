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

namespace EmojicodeCompiler {

Type ASTMethodable::analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee) {
    Type otype = callee->analyse(analyser, TypeExpectation());
    return analyseMethodCall(analyser, name, callee, otype);

}

Type ASTMethodable::analyseMethodCall(FunctionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee, const Type &otype) {
    Type type = otype.resolveOnSuperArgumentsAndConstraints(analyser->typeContext());
    if (builtIn(analyser, type, name)) {
        analyser->comply(otype, TypeExpectation(false, false), &callee);
    }
    else {
        calleeType_ = analyser->comply(otype, TypeExpectation(true, false), &callee).resolveOnSuperArgumentsAndConstraints(analyser->typeContext());
    }

    if (type.type() == TypeType::MultiProtocol) {
        return analyseMultiProtocolCall(analyser, name, type);
    }

    if (type.type() == TypeType::ValueType) {
        callType_ = CallType::StaticDispatch;
    }
    else if (type.type() == TypeType::Protocol) {
        callType_ = CallType::DynamicProtocolDispatch;
    }
    else if (type.type() == TypeType::Enum) {
        callType_ = CallType::StaticDispatch;
    }
    else if (type.type() == TypeType::Class) {
        callType_ = CallType::DynamicDispatch;
    }
    else {
        auto typeString = type.toString(analyser->typeContext());
        throw CompilerError(position(), typeString, " does not provide methods.");
    }

    auto method = type.typeDefinition()->getMethod(name, type, analyser->typeContext(), args_.isImperative(),
                                                   position());

    if (type.type() == TypeType::Class && (method->accessLevel() == AccessLevel::Private || type.isExact())) {
        callType_ = CallType::StaticDispatch;
    }

    checkMutation(analyser, callee, type, method);
    return analyser->analyseFunctionCall(&args_, type, method);
}

void ASTMethodable::checkMutation(FunctionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee, const Type &type,
                                  const Function *method) const {
    if (type.type() == TypeType::ValueType && method->mutating()) {
        if (!type.isMutable()) {
            analyser->compiler()->error(CompilerError(position(), utf8(method->name()),
                                                      " was marked 🖍 but callee is not mutable."));
        }
        else if (auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(callee)) {
            analyser->scoper().currentScope().getLocalVariable(varNode->name()).mutate(position());
        }
    }
}

Type ASTMethodable::analyseMultiProtocolCall(FunctionAnalyser *analyser, const std::u32string &name, const Type &type) {
    for (auto &protocol : type.protocols()) {
        Function *method;
        if ((method = protocol.protocol()->lookupMethod(name, args_.isImperative())) != nullptr) {
            callType_ = CallType::DynamicProtocolDispatch;
            calleeType_ = protocol;
            return analyser->analyseFunctionCall(&args_, protocol, method);
        }
    }
    throw CompilerError(position(), "No type in ", type.toString(analyser->typeContext()),
                        " provides a method called ", utf8(name), ".");
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
