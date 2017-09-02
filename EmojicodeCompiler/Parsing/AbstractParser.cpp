//
//  AbstractParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "AbstractParser.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeContext.hpp"
#include "../Parsing/CompatibleFunctionParser.hpp"
#include "CompatibilityInfoProvider.hpp"
#include <map>
#include <vector>

namespace EmojicodeCompiler {

TypeIdentifier AbstractParser::parseTypeIdentifier() {
    if (stream_.nextTokenIs(TokenType::Variable)) {
        throw CompilerError(stream_.consumeToken().position(), "Generic variables not allowed here.");
    }

    if (stream_.nextTokenIs(E_CANDY)) {
        throw CompilerError(stream_.consumeToken().position(), "Unexpected 🍬.");
    }

    std::u32string enamespace;

    if (stream_.consumeTokenIf(E_ORANGE_TRIANGLE)) {
        enamespace = stream_.consumeToken(TokenType::Identifier).value();
    }
    else {
        enamespace = kDefaultNamespace;
    }

    auto &typeName = stream_.consumeToken(TokenType::Identifier);
    return TypeIdentifier(typeName.value(), enamespace, typeName.position());
}

std::unique_ptr<FunctionParser> AbstractParser::factorFunctionParser(Package *pkg, TokenStream &stream,
                                                                     TypeContext context, Function *function) {
    if (package_->compatibilityMode()) {
        package_->compatibilityInfoProvider()->selectFunction(function);
        return std::make_unique<CompatibleFunctionParser>(pkg, stream, context);
    }
    return std::make_unique<FunctionParser>(pkg, stream, context);
}

Type AbstractParser::parseType(const TypeContext &typeContext, TypeDynamism dynamism) {
    if (stream_.nextTokenIs(E_MEDIUM_BLACK_CIRCLE)) {
        throw CompilerError(stream_.consumeToken().position(), "⚫️ not allowed here.");
    }
    if (stream_.nextTokenIs(E_WHITE_SQUARE_BUTTON)) {
        auto &token = stream_.consumeToken();
        Type type = parseType(typeContext, dynamism);
        if (!type.allowsMetaType() || type.meta()) {
            throw CompilerError(token.position(), "Meta type of ", type.toString(typeContext), " is restricted.");
        }
        type.setMeta(true);
        return type;
    }

    bool optional = stream_.consumeTokenIf(E_CANDY);

    if ((dynamism & TypeDynamism::GenericTypeVariables) != TypeDynamism::None &&
        (typeContext.calleeType().canHaveGenericArguments() || typeContext.function() != nullptr) &&
        stream_.nextTokenIs(TokenType::Variable)) {
        return parseGenericVariable(optional, typeContext, dynamism);
    }
    if (stream_.nextTokenIs(E_BENTO_BOX)) {
        return parseMultiProtocol(optional, typeContext, dynamism);
    }
    if (stream_.nextTokenIs(E_POLICE_CARS_LIGHT)) {
        return parseErrorType(optional, typeContext, dynamism);
    }
    if (stream_.consumeTokenIf(E_GRAPES)) {
        return parseCallableType(optional, typeContext, dynamism);
    }

    auto parsedTypeIdentifier = parseTypeIdentifier();
    auto type = package_->getRawType(parsedTypeIdentifier, optional);
    if (type.canHaveGenericArguments()) {
        parseGenericArgumentsForType(&type, typeContext, dynamism, parsedTypeIdentifier.position);
    }
    return type;
}

Type AbstractParser::parseMultiProtocol(bool optional, const TypeContext &typeContext, TypeDynamism dynamism) {
    auto &bentoToken = stream_.consumeToken(TokenType::Identifier);
    Type type = Type(TypeType::MultiProtocol, optional);
    while (stream_.nextTokenIsEverythingBut(E_BENTO_BOX)) {
        auto protocolType = parseType(typeContext, dynamism);
        if (protocolType.type() != TypeType::Protocol || protocolType.optional() || protocolType.meta()) {
            throw CompilerError(bentoToken.position(), "🍱 may only consist of non-optional protocol types.");
        }
        type.genericArguments_.push_back(protocolType);
    }
    if (type.protocols().empty()) {
        throw CompilerError(bentoToken.position(), "An empty 🍱 is invalid.");
    }
    type.sortMultiProtocolType();
    stream_.consumeToken(TokenType::Identifier);
    return type;
}

Type AbstractParser::parseCallableType(bool optional, const TypeContext &typeContext, TypeDynamism dynamism) {
    Type t = Type::callableIncomplete(optional);
    t.genericArguments_.push_back(Type::nothingness());

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON) &&
           stream_.nextTokenIsEverythingBut(E_RIGHTWARDS_ARROW, TokenType::Operator)) {
        t.genericArguments_.push_back(parseType(typeContext, dynamism));
    }

    if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW, TokenType::Operator)) {
        t.genericArguments_[0] = parseType(typeContext, dynamism);
    }

    stream_.requireIdentifier(E_WATERMELON);
    return t;
}

Type AbstractParser::parseGenericVariable(bool optional, const TypeContext &typeContext, TypeDynamism dynamism) {
    auto &varToken = stream_.consumeToken(TokenType::Variable);

    Type type = Type::nothingness();
    if (typeContext.function() != nullptr && typeContext.function()->fetchVariable(varToken.value(), optional, &type)) {
        return type;
    }
    if (typeContext.calleeType().canHaveGenericArguments() &&
        typeContext.calleeType().typeDefinition()->fetchVariable(varToken.value(), optional, &type)) {
        return type;
    }

    throw CompilerError(varToken.position(), "No such generic type variable \"", utf8(varToken.value()), "\".");
}

Type AbstractParser::parseErrorEnumType(const TypeContext &typeContext, TypeDynamism dynamism, const SourcePosition &p) {
    auto errorType = parseType(typeContext, dynamism);
    if (errorType.type() != TypeType::Enum || errorType.optional() || errorType.meta()) {
        throw CompilerError(p, "Error type must be a non-optional 🦃.");
    }
    return errorType;
}

Type AbstractParser::parseErrorType(bool optional, const TypeContext &typeContext, TypeDynamism dynamism) {
    auto &token = stream_.consumeToken(TokenType::Identifier);
    Type errorType = parseErrorEnumType(typeContext, dynamism, token.position());
    if (optional) {
        throw CompilerError(token.position(), "The error type itself cannot be an optional. "
                            "Maybe you meant to make the contained type an optional?");
    }
    Type type = Type::error();
    type.genericArguments_.emplace_back(errorType);
    type.genericArguments_.emplace_back(parseType(typeContext, dynamism));
    return type;
}

void AbstractParser::parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism,
                                                  const SourcePosition &p) {
    auto typeDef = type->typeDefinition();
    auto offset = typeDef->superGenericArguments().size();
    type->genericArguments_ = typeDef->superGenericArguments();

    size_t count = 0;
    for (; stream_.nextTokenIs(E_SPIRAL_SHELL) && count < typeDef->genericParameterCount(); count++) {
        auto &token = stream_.consumeToken(TokenType::Identifier);

        Type argument = parseType(typeContext, dynamism);
        if (!argument.compatibleTo(typeDef->constraintForIndex(offset + count), typeContext)) {
            throw CompilerError(token.position(), "Generic argument for ", argument.toString(typeContext),
                                " is not compatible to constraint ",
                                typeDef->constraintForIndex(offset + count).toString(typeContext), ".");
        }
        type->genericArguments_.emplace_back(argument);
    }

    if (count != typeDef->genericParameterCount()) {
        throw CompilerError(p, "Type ", type->toString(Type::nothingness()), " requires ",
                            typeDef->genericParameterCount(), " generic arguments, but ", count, " were given.");
    }
}

void AbstractParser::parseParameters(Function *function, const TypeContext &typeContext, bool initializer) {
    bool argumentToVariable;

    while ((argumentToVariable = stream_.nextTokenIs(E_BABY_BOTTLE)) || stream_.nextTokenIs(TokenType::Variable)) {
        if (argumentToVariable) {
            auto &token = stream_.consumeToken(TokenType::Identifier);
            if (!initializer) {
                throw CompilerError(token.position(), "🍼 can only be used with initializers.");
            }
        }

        auto &variableToken = stream_.consumeToken(TokenType::Variable);
        auto type = parseType(typeContext, TypeDynamism::GenericTypeVariables);

        function->arguments.emplace_back(variableToken.value(), type);

        if (argumentToVariable) {
            dynamic_cast<Initializer *>(function)->addArgumentToVariable(variableToken.value(), variableToken.position());
        }
    }

    if (function->arguments.size() > UINT8_MAX) {
        throw CompilerError(function->position(), "A function cannot take more than 255 arguments.");
    }
}

void AbstractParser::parseReturnType(Function *function, const TypeContext &typeContext) {
    if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW, TokenType::Operator)) {
        function->returnType = parseType(typeContext, TypeDynamism::GenericTypeVariables);
    }
}

}  // namespace EmojicodeCompiler
