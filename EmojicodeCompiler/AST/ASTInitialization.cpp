//
//  ASTInitialization.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/CallCodeGenerator.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Types/Enum.hpp"
#include "ASTProxyExpr.hpp"

namespace EmojicodeCompiler {

CGScoper& ASTVTInitDest::scoper(FnCodeGenerator *fncg) const {
    return inInstanceScope_ ? fncg->instanceScoper() : fncg->scoper();
}

void ASTVTInitDest::initialize(FnCodeGenerator *fncg) {
    scoper(fncg).getVariable(varId_).initialize(fncg->wr().count());
}

void ASTVTInitDest::generateExpr(FnCodeGenerator *fncg) const {
    if (declare_) {
        scoper(fncg).declareVariable(varId_, expressionType());
    }
    fncg->pushVariableReference(scoper(fncg).getVariable(varId_).stackIndex, inInstanceScope_);
}

void ASTInitialization::generateExpr(FnCodeGenerator *fncg) const {
    switch (initType_) {
        case InitType::Class:
            InitializationCallCodeGenerator(fncg, INS_NEW_OBJECT).generate(*typeExpr_, typeExpr_->expressionType(),
                                                                           args_, name_);
            break;
        case InitType::Enum:
            fncg->writeInteger(typeExpr_->expressionType().eenum()->getValueFor(name_).second);
            break;
        case InitType::ValueType:
            VTInitializationCallCodeGenerator(fncg).generate(vtDestination_, typeExpr_->expressionType(), args_, name_);
    }
}

Type ASTInitialization::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    if (type.type() == TypeType::Enum) {
        initType_ = InitType::Enum;
        notStaticError(typeExpr_->availability(), position(), "Enums");

        auto v = type.eenum()->getValueFor(name_);
        if (!v.first) {
            throw CompilerError(position(), type.toString(analyser->typeContext()), " does not have a member named ",
                                utf8(name_), ".");
        }
        return type;
    }

    if (type.type() == TypeType::ValueType) {
        initType_ = InitType::ValueType;
        type.setMutable(expectation.isMutable());
        notStaticError(typeExpr_->availability(), position(), "Value Types");
    }

    auto initializer = type.typeDefinition()->getInitializer(name_, type, analyser->typeContext(), position());
    analyser->analyseFunctionCall(&args_, type, initializer);
    return initializer->constructedType(type);
}

}  // namespace EmojicodeCompiler
