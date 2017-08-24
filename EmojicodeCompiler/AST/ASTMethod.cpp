//
//  ASTMethod.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTMethod.hpp"
#include "../../EmojicodeInstructions.h"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Protocol.hpp"

namespace EmojicodeCompiler {

Type ASTMethodable::analyseMethodCall(SemanticAnalyser *analyser, const std::u32string &name,
                                      std::shared_ptr<ASTExpr> &callee) {
    if (name.front() == E_NEGATIVE_SQUARED_CROSS_MARK) {
        invert_ = true;
        analyser->expectType(Type::boolean(), &callee);
        return Type::boolean();
    }
    
    Type type = analyser->expect(TypeExpectation(true, false), &callee)
    .resolveOnSuperArgumentsAndConstraints(analyser->typeContext());

    if (type.type() == TypeType::MultiProtocol) {
        for (auto &protocol : type.protocols()) {
            Function *method;
            if ((method = protocol.protocol()->lookupMethod(name)) != nullptr) {
                instruction_ = INS_DISPATCH_PROTOCOL;
                callee->setExpressionType(protocol);
                return analyser->analyseFunctionCall(&args_, protocol, method);
            }
        }
        throw CompilerError(position(), "No type in %s provides a method called %s.",
                            type.toString(analyser->typeContext()).c_str(), utf8(name).c_str());
    }

    auto method = type.typeDefinition()->getMethod(name, type, analyser->typeContext(), position());

    if (type.type() == TypeType::ValueType) {
        if (method->mutating()) {
            if (!type.isMutable()) {
                throw CompilerError(position(), "%s was marked 🖍 but callee is not mutable.",
                                    utf8(method->name()).c_str());
            }
            auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(callee);
            assert(varNode != nullptr && varNode->reference());
            analyser->scoper().currentScope().getLocalVariable(varNode->name()).mutate(position());
        }
        instruction_ = INS_CALL_CONTEXTED_FUNCTION;
    }
    else if (type.type() == TypeType::Protocol) {
        instruction_ = INS_DISPATCH_PROTOCOL;
    }
    else if (type.type() == TypeType::Enum) {
        instruction_ = INS_CALL_CONTEXTED_FUNCTION;
    }
    else if (type.type() == TypeType::Class) {
        instruction_ = INS_DISPATCH_METHOD;
    }
    else {
        auto typeString = type.toString(analyser->typeContext());
        throw CompilerError(position(), "You cannot call methods on %s.", typeString.c_str());
    }
    return analyser->analyseFunctionCall(&args_, type, method);
}

void ASTMethod::generateExpr(FnCodeGenerator *fncg) const {
    if (invert_) {
        callee_->generate(fncg);
        fncg->wr().writeInstruction(INS_INVERT_BOOLEAN);
        return;
    }
    CallCodeGenerator(fncg, instruction_).generate(*callee_, args_, name_);
}

Type ASTMethod::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return analyseMethodCall(analyser, name_, callee_);
}

}  // namespace EmojicodeCompiler
