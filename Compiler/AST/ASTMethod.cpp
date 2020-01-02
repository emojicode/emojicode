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
#include "Analysis/SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "Emojis.h"
#include "Functions/Function.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTMethodable::analyseMethodCall(ExpressionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee) {
    return analyseMethodCall(analyser, name, callee, analyser->analyse(callee));

}

Type ASTMethodable::analyseMethodCall(ExpressionAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee, const Type &otype) {
    determineCalleeType(analyser, name, callee, otype);

    if (calleeType_.unboxedType() == TypeType::MultiProtocol) {
        return analyseMultiProtocolCall(analyser, name);
    }
    if (calleeType_.type() == TypeType::TypeAsValue) {
        return analyseTypeMethodCall(analyser, name, callee);
    }

    determineCallType(analyser);

    method_ = calleeType_.typeDefinition()->methods().get(name, args_.mood(), &args_,
                                                          &calleeType_, analyser, position());

    if (calleeType_.type() == TypeType::Class && (method_->accessLevel() == AccessLevel::Private || calleeType_.isExact())) {
        callType_ = CallType::StaticDispatch;
    }

    checkMutation(analyser, callee);
    ensureErrorIsHandled(analyser);
    auto rt = analyser->analyseFunctionCall(&args_, calleeType_, method_);
    if (method_->owner() != analyser->compiler()->sMemory &&
        (method_->returnType()->type().is<TypeType::GenericVariable>() ||
         method_->returnType()->type().is<TypeType::LocalGenericVariable>() ||
         method_->returnType()->type().unoptionalized().is<TypeType::GenericVariable>() ||
         method_->returnType()->type().unoptionalized().is<TypeType::LocalGenericVariable>())) {  // i.e. not boxed
        castTo_ = rt;
    }
    return rt;
}

const Type& ASTMethodable::errorType() const {
    return method_->errorType()->type();
}

bool ASTMethodable::isErrorProne() const {
    return method_->errorProne();
}

void ASTMethodable::determineCalleeType(ExpressionAnalyser *analyser, const std::u32string &name,
                                        std::shared_ptr<ASTExpr> &callee, const Type &otype) {
    Type type = analyser->semanticAnalyser()->defaultLiteralType(otype.resolveOnSuperArgumentsAndConstraints(analyser->typeContext()));
    if (builtIn(analyser, type, name)) {
        calleeType_ = analyser->comply(TypeExpectation(false, false), &callee);
    }
    else {
        calleeType_ = analyser->comply(TypeExpectation(true, false),
                                       &callee).resolveOnSuperArgumentsAndConstraints(analyser->typeContext());
    }
}

void ASTMethodable::determineCallType(const ExpressionAnalyser *analyser) {
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

void ASTMethodable::checkMutation(ExpressionAnalyser *analyser, const std::shared_ptr<ASTExpr> &callee) const {
    if (calleeType_.type() == TypeType::ValueType && method_->mutating()) {
        try {
            callee->mutateReference(analyser);
            if (!calleeType_.isMutable()) {
                analyser->compiler()->error(CompilerError(position(), utf8(method_->name()),
                                                          " was marked 🖍 but callee is not mutable."));
            }
        }
        catch (CompilerError &err) {
            analyser->compiler()->error(err);
        }
    }
}

void ASTMethod::mutateReference(ExpressionAnalyser *analyser) {
    callee_->mutateReference(analyser);
}

Type ASTMethodable::analyseTypeMethodCall(ExpressionAnalyser *analyser, const std::u32string &name,
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

    method_ = calleeType_.typeDefinition()->typeMethods().get(name, args_.mood(), &args_,
                                                              &calleeType_, analyser, position());

    if (calleeType_.type() == TypeType::Class && (method_->accessLevel() == AccessLevel::Private || calleeType_.isExact())) {
        callType_ = CallType::StaticDispatch;
    }
    ensureErrorIsHandled(analyser);
    return analyser->analyseFunctionCall(&args_, calleeType_, method_);
}

Type ASTMethodable::analyseMultiProtocolCall(ExpressionAnalyser *analyser, const std::u32string &name) {
    auto resolution = FunctionResolution<Function>(name, args_.mood(), &args_, calleeType_, analyser);
    for (auto &protocol : calleeType_.protocols()) {
        resolution.addResolver(&protocol.protocol()->methods());
    }
    if ((method_ = resolution.resolveAndReificate(&args_, &calleeType_)) != nullptr) {
        for (; multiprotocolN_ < calleeType_.protocols().size(); multiprotocolN_++) {
            if (calleeType_.protocols()[multiprotocolN_].protocol() == method_->owner()) {
                break;
            }
        }
        builtIn_ = BuiltInType::Multiprotocol;
        callType_ = CallType::DynamicProtocolDispatch;
        return analyser->analyseFunctionCall(&args_, calleeType_, method_);
    }
    throw CompilerError(position(), "No type in ", calleeType_.toString(analyser->typeContext()),
                        " provides a method ", utf8(name), ".");
}

std::map<std::pair<TypeDefinition*, char32_t>, ASTMethodable::BuiltInType> ASTMethodable::kBuiltIns = {};

void ASTMethodable::prepareBuiltIns(Compiler *c) {
    if (!kBuiltIns.empty()) return;
    kBuiltIns = {
        {{c->sBoolean, E_NEGATIVE_SQUARED_CROSS_MARK}, BuiltInType::BooleanNegate},
        {{c->sInteger, E_HUNDRED_POINTS_SYMBOL}, BuiltInType::IntegerToDouble},
        {{c->sInteger, E_NEGATIVE_SQUARED_CROSS_MARK}, BuiltInType::IntegerNot},
        {{c->sInteger, E_BATTERY}, BuiltInType::IntegerInverse},
        {{c->sInteger, 0x1f4a7}, BuiltInType::IntegerToByte},
        {{c->sByte, E_NEGATIVE_SQUARED_CROSS_MARK}, BuiltInType::IntegerNot},
        {{c->sByte, E_BATTERY}, BuiltInType::IntegerInverse},
        {{c->sByte, 0x1f522}, BuiltInType::ByteToInteger},
        {{c->sReal, E_BATTERY}, BuiltInType::DoubleInverse},
        {{c->sReal, 0x1F3C2}, BuiltInType::Power},
        {{c->sReal, 0x1f6a3}, BuiltInType::Log2},
        {{c->sReal, 0x1f94f}, BuiltInType::Log10},
        {{c->sReal, 0x1f3c4}, BuiltInType::Ln},
        {{c->sReal, 0x1f6b5}, BuiltInType::Floor},
        {{c->sReal, 0x1f6b4}, BuiltInType::Ceil},
        {{c->sReal, 0x1f3c7}, BuiltInType::Round},
        {{c->sReal, 0x1f3e7}, BuiltInType::DoubleAbs},
        {{c->sReal, 0x1f522}, BuiltInType::DoubleToInteger},
        {{c->sMemory, E_RECYCLING_SYMBOL}, BuiltInType::Release},
        {{c->sMemory, 0x1F69C}, BuiltInType::MemoryMove},
        {{c->sMemory, 0x270D}, BuiltInType::MemorySet},
        {{c->sMemory, 0x1F43D}, BuiltInType::Load},
    };
}

bool ASTMethodable::builtIn(ExpressionAnalyser *analyser, const Type &btype, const std::u32string &name) {
    auto type = btype.unboxed();
    if (type.type() != TypeType::ValueType) {
        return false;
    }

    if (args_.mood() == Mood::Assignment && type.typeDefinition() == analyser->compiler()->sMemory
        && name.front() == 0x1F43D) {
        builtIn_ = BuiltInType::Store;
        return true;
    }

    prepareBuiltIns(analyser->compiler());
    auto it = kBuiltIns.find(std::make_pair(type.valueType(), name.front()));
    if (it != kBuiltIns.end()) {
        builtIn_ = it->second;
        return true;
    }

    return false;
}

Type ASTMethod::analyse(ExpressionAnalyser *analyser) {
    return analyseMethodCall(analyser, name_, callee_);
}

void ASTMethod::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->analyseFunctionCall(&args_, callee_.get(), method_);
}

}  // namespace EmojicodeCompiler
