//
//  ASTMethod.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "ASTVariables.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"

namespace EmojicodeCompiler {

Type ASTMethodable::analyseMethodCall(SemanticAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee) {
    Type otype = callee->analyse(analyser, TypeExpectation());
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

    auto method = type.typeDefinition()->getMethod(name, type, analyser->typeContext(), args_.isImperative(),
                                                   position());

    if (type.type() == TypeType::ValueType) {
        if (method->mutating()) {
            if (!type.isMutable()) {
                analyser->compiler()->error(CompilerError(position(), utf8(method->name()),
                                                          " was marked 🖍 but callee is not mutable."));
            }
            if (auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(callee)) {
                analyser->scoper().currentScope().getLocalVariable(varNode->name()).mutate(position());
            }
        }
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
        throw CompilerError(position(), "You cannot call methods on ", typeString, ".");
    }
    return analyser->analyseFunctionCall(&args_, type, method);
}

Type ASTMethodable::analyseMultiProtocolCall(SemanticAnalyser *analyser, const std::u32string &name, const Type &type) {
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

bool ASTMethodable::builtIn(SemanticAnalyser *analyser, const Type &type, const std::u32string &name) {
    if (type.typeDefinition() == analyser->compiler()->sBoolean) {
        if (name.front() == E_NEGATIVE_SQUARED_CROSS_MARK) {
            builtIn_ = BuiltInType::BooleanNegate;
            return true;
        }
    }
    else if (type.typeDefinition() == analyser->compiler()->sInteger) {
        if (name.front() == E_ROCKET) {
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

Type ASTMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return analyseMethodCall(analyser, name_, callee_);
}

}  // namespace EmojicodeCompiler
