//
//  ASTLiterals.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTLiterals.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Compiler.hpp"
#include "../Parsing/AbstractParser.hpp"
#include "../Types/CommonTypeFinder.hpp"
#include "../Types/TypeExpectation.hpp"
#include "../Types/Class.hpp"

namespace EmojicodeCompiler {

Type ASTStringLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type(CL_STRING, false);
}

Type ASTBooleanTrue::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::boolean();
}

Type ASTBooleanFalse::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::boolean();
}

Type ASTNumberLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeType::ValueType && expectation.valueType() == VT_DOUBLE
        && type_ == NumberType::Integer) {
        type_ = NumberType::Double;
        doubleValue_ = integerValue_;
    }

    switch (type_) {
        case NumberType::Integer:
            return Type::integer();
        case NumberType::Double:
            return Type::doubl();
    }
}

Type ASTSymbolLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return Type::symbol();
}

Type ASTThis::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (isSuperconstructorRequired(analyser->function()->functionType()) &&
        !analyser->pathAnalyser().hasCertainly(PathAnalyserIncident::CalledSuperInitializer) &&
        analyser->typeContext().calleeType().eclass()->superclass() != nullptr) {
        analyser->compiler()->error(CompilerError(position(), "Attempt to use 🐕 before superinitializer call."));
    }
    if (isFullyInitializedCheckRequired(analyser->function()->functionType())) {
        analyser->scoper().instanceScope()->unintializedVariablesCheck(position(), "Instance variable \"",
                                                                       "\" must be initialized before using 🐕.");
    }

    if (!isSelfAllowed(analyser->function()->functionType())) {
        throw CompilerError(position(), "Illegal use of 🐕.");
    }
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    return analyser->typeContext().calleeType();
}

Type ASTNothingness::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (!expectation.optional() && expectation.type() != TypeType::Something) {
        throw CompilerError(position(), "⚡ can only be used when an optional is expected.");
    }
    type_ = expectation.copyType();
    return type_;
}

Type ASTDictionaryLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = Type(CL_DICTIONARY, false);

    CommonTypeFinder finder;
    for (auto it = values_.begin(); it != values_.end(); it++) {
        analyser->expectType(Type(CL_STRING, false), &*it);
        if (++it == values_.end()) {
            throw CompilerError(position(), "A value must be provided for every key.");
        }
        finder.addType(analyser->expect(TypeExpectation(false, true, false), &*it), analyser->typeContext());
    }

    type_.setGenericArgument(0, finder.getCommonType(position(), analyser->compiler()));
    return type_;
}

Type ASTListLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = Type(CL_LIST, false);

    CommonTypeFinder finder;
    for (auto &valueNode : values_) {
        Type type = analyser->expect(TypeExpectation(false, true, false), &valueNode);
        finder.addType(type, analyser->typeContext());
    }

    type_.setGenericArgument(0, finder.getCommonType(position(), analyser->compiler()));
    return type_;
}

Type ASTConcatenateLiteral::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = analyser->function()->package()->getRawType(TypeIdentifier(std::u32string(1, 0x1F520), kDefaultNamespace,
                                                                       position()), false);

    for (auto &stringNode : values_) {
        analyser->expectType(Type(CL_STRING, false), &stringNode);
    }
    return Type(CL_STRING, false);
}

}  // namespace EmojicodeCompiler
