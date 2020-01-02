//
//  ASTInitialization.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "ASTTypeExpr.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Emojis.h"
#include "Functions/Initializer.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/Enum.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTInitialization::analyse(ExpressionAnalyser *analyser) {
    auto type = analyser->analyseTypeExpr(typeExpr_, TypeExpectation(), true);

    if (type.type() == TypeType::Enum) {
        return analyseEnumInit(analyser, type);
    }

    if (type.type() == TypeType::ValueType) {
        initType_ = type.valueType() == analyser->compiler()->sMemory ? InitType::MemoryAllocation : InitType::ValueType;
    }

    auto init = type.typeDefinition()->inits().get(name_, Mood::Imperative, &args_, &type, analyser, position());
    if (!type.isExact()) {
        if (!init->required()) {
            throw CompilerError(position(), "Type is not exact; can only use required initializer.");
        }
        initializer_ = type.typeDefinition()->typeMethods().get(std::u32string({ E_KEY }) + name_, Mood::Imperative,
                                                                &args_, &type, analyser, position());
    }
    else {
        initializer_ = init;
    }

    analyser->analyseFunctionCall(&args_, type, init);
    ensureErrorIsHandled(analyser);
    return init->constructedType(type);
}

Type ASTInitialization::comply(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (typeExpr_->expressionType().type() == TypeType::ValueType) {
        auto type = expressionType();
        type.setMutable(expectation.isMutable());
        return static_cast<Initializer*>(initializer_)->constructedType(type);
    }
    return expressionType();
}

const Type& ASTInitialization::errorType() const {
    return initializer_->errorType()->type();
}

bool ASTInitialization::isErrorProne() const {
    return initializer_->errorProne();
}

Type ASTInitialization::analyseEnumInit(ExpressionAnalyser *analyser, Type &type) {
    initType_ = InitType::Enum;

    auto v = type.enumeration()->getValueFor(name_);
    if (!v.first) {
        throw CompilerError(position(), type.toString(analyser->typeContext()), " does not have a member named ",
                            utf8(name_), ".");
    }
    return type;
}

void ASTInitialization::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    if (initType_ == InitType::Enum) {
        return;
    }
    if (!type.isEscaping() && initType_ == InitType::Class && !initializer_->memoryFlowTypeForThis().isEscaping()) {
        initType_ = InitType::ClassStack;
    }
    analyser->analyseFunctionCall(&args_, typeExpr_.get(), initializer_);
}

void ASTInitialization::allocateOnStack() {
    if (initType() == InitType::Class && !initializer_->memoryFlowTypeForThis().isEscaping()) {
        initType_ = InitType::ClassStack;
    }
}

}  // namespace EmojicodeCompiler
