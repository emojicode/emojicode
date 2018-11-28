//
//  ASTLiterals.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTLiterals.hpp"
#include "Analysis/FunctionAnalyser.hpp"
#include "Compiler.hpp"
#include "MemoryFlowAnalysis/MFFunctionAnalyser.hpp"
#include "Package/Package.hpp"
#include "Parsing/AbstractParser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

Type ASTStringLiteral::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = Type(analyser->compiler()->sString);
    type.setExact(true);
    return type;
}

Type ASTBooleanTrue::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    return analyser->boolean();
}

Type ASTBooleanFalse::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    return analyser->boolean();
}

Type ASTNumberLiteral::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (type_ == NumberType::Integer) {
        if (expectation == analyser->real()) {
            type_ = NumberType::Double;
            doubleValue_ = integerValue_;
            return analyser->real();
        }
        if (expectation == analyser->byte()) {
            if (integerValue_ > 255) {
                analyser->compiler()->warn(position(), "Literal implicitly is a byte integer literal but value does ",
                                           "not fit into byte type.");
            }
            type_ = NumberType::Byte;
            return analyser->byte();
        }
        return analyser->integer();
    }
    return analyser->real();
}

Type ASTThis::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    analyser->checkThisUse(position());

    if (!isSelfAllowed(analyser->functionType())) {
        throw CompilerError(position(), "Illegal use of 🐕.");
    }
    analyser->pathAnalyser().recordIncident(PathAnalyserIncident::UsedSelf);
    return analyser->typeContext().calleeType();
}

void ASTThis::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    analyser->recordThis(type);
}

Type ASTNoValue::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.unboxedType() != TypeType::Optional && expectation.unboxedType() != TypeType::Something) {
        throw CompilerError(position(), "🤷‍ can only be used when an optional is expected.");
    }
    type_ = expectation.copyType();
    return type_;
}

Type ASTDictionaryLiteral::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = Type(analyser->compiler()->sDictionary);
    type_.typeDefinition()->lookupInitializer(U"🐴")->createUnspecificReification();
    type_.typeDefinition()->lookupMethod(U"🐷", true)->createUnspecificReification();

    CommonTypeFinder finder;
    for (auto it = values_.begin(); it != values_.end(); it++) {
        analyser->expectType(Type(analyser->compiler()->sString), &*it);
        if (++it == values_.end()) {
            throw CompilerError(position(), "A value must be provided for every key.");
        }
        finder.addType(analyser->expect(TypeExpectation(), &(*it)), analyser->typeContext());
    }

    type_.setGenericArgument(0, finder.getCommonType(position(), analyser->compiler()));
    type_.setExact(true);

    auto elementType = analyser->compiler()->sList->typeForVariable(0).resolveOn(TypeContext(type_));
    for (auto it = values_.begin() + 1; it - 1 != values_.end(); it += 2) {
        analyser->comply((*it)->expressionType(), TypeExpectation(elementType), &(*it));
    }

    return type_;
}

void ASTDictionaryLiteral::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    for (auto &valueNode : values_) {
        valueNode->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);
    }
}

Type ASTListLiteral::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = Type(analyser->compiler()->sList);
    type_.typeDefinition()->lookupInitializer(U"🐴")->createUnspecificReification();
    type_.typeDefinition()->lookupMethod(U"🐻", true)->createUnspecificReification();
        
    if (expectation.type() == TypeType::ValueType && expectation.valueType() == analyser->compiler()->sList) {
        auto type = analyser->compiler()->sList->typeForVariable(0).resolveOn(TypeContext(expectation.copyType()));
        for (auto &valueNode : values_) {
            analyser->expectType(type, &valueNode);
        }
        type_ = expectation.copyType();
        type_.setExact(true);
        return type_;
    }

    CommonTypeFinder finder;
    for (auto &valueNode : values_) {
        Type type = analyser->expect(TypeExpectation(), &valueNode);
        finder.addType(type, analyser->typeContext());
    }

    type_.setGenericArgument(0, finder.getCommonType(position(), analyser->compiler()));
    type_.setExact(true);

    auto elementType = analyser->compiler()->sList->typeForVariable(0).resolveOn(TypeContext(type_));
    for (auto &valueNode : values_) {
        analyser->comply(valueNode->expressionType(), TypeExpectation(elementType), &valueNode);
    }

    return type_;
}

void ASTListLiteral::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    for (auto &valueNode : values_) {
        valueNode->analyseMemoryFlow(analyser, MFFlowCategory::Escaping);
    }
}

Type ASTConcatenateLiteral::analyse(ExpressionAnalyser *analyser, const TypeExpectation &expectation) {
    type_ = analyser->package()->getRawType(TypeIdentifier(U"🔠", kDefaultNamespace, position()));

    auto stringType = Type(analyser->compiler()->sString);
    for (auto &stringNode : values_) {
        analyser->expectType(stringType, &stringNode);
    }
    type_.setExact(true);
    return stringType;
}

void ASTConcatenateLiteral::analyseMemoryFlow(MFFunctionAnalyser *analyser, MFFlowCategory type) {
    for (auto &valueNode : values_) {
        valueNode->analyseMemoryFlow(analyser, MFFlowCategory::Borrowing);
    }
}

}  // namespace EmojicodeCompiler
