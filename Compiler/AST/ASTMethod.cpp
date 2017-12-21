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
    auto pair = builtIn(analyser, type, name);
    if (pair.first) {
        analyser->comply(otype, TypeExpectation(false, false), &callee);
        return pair.second;
    }
    calleeType_ = analyser->comply(otype, TypeExpectation(true, false), &callee).resolveOnSuperArgumentsAndConstraints(analyser->typeContext());

    if (type.type() == TypeType::MultiProtocol) {
        for (auto &protocol : type.protocols()) {
            Function *method;
            if ((method = protocol.protocol()->lookupMethod(name)) != nullptr) {
                callType_ = CallType::DynamicProtocolDispatch;
                calleeType_ = protocol;
                return analyser->analyseFunctionCall(&args_, protocol, method);
            }
        }
        throw CompilerError(position(), "No type in ", type.toString(analyser->typeContext()),
                            " provides a method called ", utf8(name), ".");
    }

    auto method = type.typeDefinition()->getMethod(name, type, analyser->typeContext(), position());

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

std::pair<bool, Type> ASTMethodable::builtIn(SemanticAnalyser *analyser, const Type &type, const std::u32string &name) {
    if (type.typeDefinition() == analyser->compiler()->sBoolean) {
        if (name.front() == E_NEGATIVE_SQUARED_CROSS_MARK) {
            builtIn_ = BuiltInType::BooleanNegate;
            return std::make_pair(true, analyser->boolean());
        }
    }
    else if (type.typeDefinition() == analyser->compiler()->sInteger) {
        if (name.front() == E_ROCKET) {
            builtIn_ = BuiltInType::IntegerToDouble;
            return std::make_pair(true, analyser->doubleType());
        }
        if (name.front() == E_NO_ENTRY_SIGN) {
            builtIn_ = BuiltInType::IntegerNot;
            return std::make_pair(true, analyser->integer());
        }
    }
    return std::make_pair(false, Type::noReturn());
}

Type ASTMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return analyseMethodCall(analyser, name_, callee_);
}

}  // namespace EmojicodeCompiler
