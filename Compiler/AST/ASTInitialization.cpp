//
//  ASTInitialization.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTInitialization.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "Emojis.h"
#include "Functions/Initializer.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Types/Enum.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTInitialization::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = analyser->analyseTypeExpr(typeExpr_, expectation);

    if (type.type() == TypeType::Enum) {
        return analyseEnumInit(analyser, type);
    }

    if (type.type() == TypeType::ValueType) {
        initType_ = type.valueType() == analyser->compiler()->sMemory ? InitType::MemoryAllocation : InitType::ValueType;

        type.setMutable(expectation.isMutable());
    }

    auto init = type.typeDefinition()->getInitializer(name_, type, analyser->typeContext(), position());
    if (!type.isExact()) {
        if (!init->required()) {
            throw CompilerError(position(), "Type is not exact; can only use required initializer.");
        }
        initializer_ = type.typeDefinition()->lookupTypeMethod(std::u32string({ E_KEY }) + name_, true);
    }
    else {
        initializer_ = init;
    }

    analyser->analyseFunctionCall(&args_, type, init);
    return init->constructedType(type);
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
    if (!type.isEscaping() && initType_ == InitType::Class) {
        initType_ = InitType::ClassStack;
    }
    analyser->analyseFunctionCall(&args_, typeExpr_.get(), initializer_);
}

void ASTInitialization::allocateOnStack() {
    if (initType() == InitType::Class) {
        initType_ = InitType::ClassStack;
    }
}

}  // namespace EmojicodeCompiler
