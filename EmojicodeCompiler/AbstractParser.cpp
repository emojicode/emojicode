//
//  AbstractParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include <vector>
#include "AbstractParser.hpp"
#include "Function.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"

ParsedType AbstractParser::parseType() {
    if (stream_.nextTokenIs(TokenType::Variable)) {
        throw CompilerError(stream_.consumeToken().position(), "Generic variables not allowed here.");
    }

    if (stream_.nextTokenIs(E_CANDY)) {
        throw CompilerError(stream_.consumeToken().position(), "Unexpected 🍬.");
    }

    EmojicodeString enamespace;

    if (stream_.consumeTokenIf(E_ORANGE_TRIANGLE)) {
        enamespace = stream_.consumeToken(TokenType::Identifier).value();
    }
    else {
        enamespace = globalNamespace;
    }

    auto &typeName = stream_.consumeToken(TokenType::Identifier);
    return ParsedType(typeName.value(), enamespace, typeName);
}

Type AbstractParser::parseTypeDeclarative(const TypeContext &ct, TypeDynamism dynamism, Type expectation,
                                          TypeDynamism *dynamicType) {
    if (stream_.nextTokenIs(E_MEDIUM_BLACK_CIRCLE)) {
        if (expectation.type() == TypeContent::Nothingness) {
            throw CompilerError(stream_.consumeToken().position(), "Cannot infer ⚫️.");
        }
        stream_.consumeToken();
        return expectation.copyWithoutOptional();
    }
    if (stream_.nextTokenIs(E_WHITE_SQUARE_BUTTON)) {
        auto &token = stream_.consumeToken();
        Type type = parseTypeDeclarative(ct, dynamism, expectation, dynamicType);
        if (!type.allowsMetaType() || type.meta()) {
            throw CompilerError(token.position(), "Meta type of %s is restricted.", type.toString(ct, true).c_str());
        }
        type.setMeta(true);
        return type;
    }

    bool optional = false;
    if (stream_.consumeTokenIf(E_CANDY)) {
        optional = true;
    }

    if ((dynamism & TypeDynamism::GenericTypeVariables) != TypeDynamism::None &&
        (ct.calleeType().canHaveGenericArguments() || ct.function()) && stream_.nextTokenIs(TokenType::Variable)) {
        if (dynamicType) *dynamicType = TypeDynamism::GenericTypeVariables;

        auto &variableToken = stream_.consumeToken(TokenType::Variable);

        if (ct.function()) {
            auto it = ct.function()->genericArgumentVariables.find(variableToken.value());
            if (it != ct.function()->genericArgumentVariables.end()) {
                Type type = it->second;
                if (optional) type.setOptional();
                return type;
            }
        }
        if (ct.calleeType().canHaveGenericArguments()) {
            Type type = Type::nothingness();
            if (ct.calleeType().typeDefinitionFunctional()->fetchVariable(variableToken.value(), optional, &type)) {
                return type;
            }
        }

        throw CompilerError(variableToken.position(), "No such generic type variable \"%s\".",
                            variableToken.value().utf8().c_str());
    }
    else if (stream_.nextTokenIs(E_DOG)) {
        auto &selfToken = stream_.consumeToken(TokenType::Identifier);
        if ((dynamism & TypeDynamism::Self) == TypeDynamism::None) {
            throw CompilerError(selfToken.position(), "🐕 not allowed here.");
        }
        if (dynamicType) *dynamicType = TypeDynamism::Self;
        return Type::self(optional);
    }
    else if (stream_.nextTokenIs(E_BENTO_BOX)) {
        auto &bentoToken = stream_.consumeToken(TokenType::Identifier);
        Type type = Type(TypeContent::MultiProtocol, optional);
        while (stream_.nextTokenIsEverythingBut(E_BENTO_BOX)) {
            auto protocolType = parseTypeDeclarative(ct, dynamism);
            if (protocolType.type() != TypeContent::Protocol || protocolType.optional() || protocolType.meta()) {
                throw CompilerError(bentoToken.position(), "🍱 may only consist of non-optional protocol types.");
            }
            type.genericArguments_.push_back(protocolType);
        }
        if (type.protocols().size() == 0) {
            throw CompilerError(bentoToken.position(), "An empty 🍱 is invalid.");
        }
        type.sortMultiProtocolType();
        stream_.consumeToken(TokenType::Identifier);
        return type;
    }
    else if (stream_.nextTokenIs(E_POLICE_CARS_LIGHT)) {
        auto &token = stream_.consumeToken(TokenType::Identifier);
        Type errorType = parseTypeDeclarative(ct, dynamism);
        if (errorType.type() != TypeContent::Enum || errorType.optional() || errorType.meta()) {
            throw CompilerError(token.position(), "Error type must be a non-optional 🦃.");
        }
        if (optional) {
            throw CompilerError(token.position(), "The error type itself cannot be an optional. "
                                "Maybe you meant to make the contained type an optional?");
        }
        Type type = Type::error();
        type.genericArguments_.emplace_back(errorType);
        type.genericArguments_.emplace_back(parseTypeDeclarative(ct, dynamism));
        return type;
    }
    else if (stream_.consumeTokenIf(E_GRAPES)) {
        if (dynamicType) *dynamicType = TypeDynamism::None;
        
        Type t = Type::callableIncomplete(optional);
        t.genericArguments_.push_back(Type::nothingness());

        while (stream_.nextTokenIsEverythingBut(E_WATERMELON) && stream_.nextTokenIsEverythingBut(E_RIGHTWARDS_ARROW)) {
            t.genericArguments_.push_back(parseTypeDeclarative(ct, dynamism));
        }

        if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW)) {
            t.genericArguments_[0] = parseTypeDeclarative(ct, dynamism);
        }

        stream_.requireIdentifier(E_WATERMELON);
        return t;
    }
    else {
        if (dynamicType) *dynamicType = TypeDynamism::None;
        auto parsedType = parseType();

        auto type = Type::nothingness();
        if (!package_->fetchRawType(parsedType, optional, &type)) {
            throw CompilerError(parsedType.token.position(), "Could not find type %s in enamespace %s.",
                                         parsedType.name.utf8().c_str(), parsedType.ns.utf8().c_str());
        }

        parseGenericArgumentsForType(&type, ct, dynamism, parsedType.token.position());

        return type;
    }
}

void AbstractParser::parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, TypeDynamism dynamism,
                                                  SourcePosition p) {
    if (type->canHaveGenericArguments()) {
        auto typeDef = type->typeDefinitionFunctional();
        auto offset = typeDef->superGenericArguments().size();
        type->genericArguments_ = typeDef->superGenericArguments();

        if (!typeDef->ownGenericArgumentVariables().empty()) {
            size_t count = 0;
            while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
                auto &token = stream_.consumeToken(TokenType::Identifier);

                Type ta = parseTypeDeclarative(typeContext, dynamism, Type::nothingness(), nullptr);

                auto i = count + offset;
                if (typeDef->numberOfGenericArgumentsWithSuperArguments() <= i) {
                    break;  // and throw an error below
                }
                if (!ta.compatibleTo(typeDef->genericArgumentConstraints()[i], typeContext)) {
                    auto thisName = typeDef->genericArgumentConstraints()[i].toString(typeContext, true);
                    auto thatName = ta.toString(typeContext, true);
                    throw CompilerError(token.position(), "Generic argument for %s is not compatible to constraint %s.",
                                        thatName.c_str(), thisName.c_str());
                }

                type->genericArguments_.push_back(ta);

                count++;
            }

            if (count != typeDef->ownGenericArgumentVariables().size()) {
                auto str = type->toString(Type::nothingness(), true);
                throw CompilerError(p, "Type %s requires %d generic arguments, but %d were given.", str.c_str(),
                                    typeDef->ownGenericArgumentVariables().size(), count);
            }
        }
    }
}

void AbstractParser::parseArgumentList(Function *function, const TypeContext &typeContext, bool initializer) {
    bool argumentToVariable;

    while ((argumentToVariable = stream_.nextTokenIs(E_BABY_BOTTLE)) || stream_.nextTokenIs(TokenType::Variable)) {
        if (argumentToVariable) {
            auto &token = stream_.consumeToken(TokenType::Identifier);
            if (!initializer) {
                throw CompilerError(token.position(), "🍼 can only be used with initializers.");
            }
        }

        TypeDynamism dynamism;
        auto &variableToken = stream_.consumeToken(TokenType::Variable);
        auto type = parseTypeDeclarative(typeContext, TypeDynamism::GenericTypeVariables, Type::nothingness(),
                                         &dynamism);

        function->arguments.push_back(Argument(variableToken.value(), type));

        if (argumentToVariable) {
            static_cast<Initializer *>(function)->addArgumentToVariable(variableToken.value());
        }
    }

    if (function->arguments.size() > UINT8_MAX) {
        throw CompilerError(function->position(), "A function cannot take more than 255 arguments.");
    }
}

void AbstractParser::parseReturnType(Function *function, const TypeContext &typeContext) {
    if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW)) {
        TypeDynamism dynamism;

        function->returnType = parseTypeDeclarative(typeContext, TypeDynamism::GenericTypeVariables,
                                                    Type::nothingness(), &dynamism);
    }
}

void AbstractParser::parseGenericArgumentsInDefinition(Function *function, const TypeContext &ct) {
    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        auto &variable = stream_.consumeToken(TokenType::Variable);

        Type t = parseTypeDeclarative(TypeContext(function->owningType(), function), TypeDynamism::GenericTypeVariables,
                                      Type::nothingness(), nullptr);
        function->genericArgumentConstraints.push_back(t);

        Type rType = Type(TypeContent::LocalReference, false,
                          static_cast<int>(function->genericArgumentVariables.size()), function);

        if (function->genericArgumentVariables.count(variable.value()) > 0) {
            throw CompilerError(variable.position(),
                                "A generic argument variable with the same name is already in use.");
        }
        function->genericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable.value(), rType));
    }
}

void AbstractParser::parseBody(Function *function, bool allowNative) {
    if (stream_.nextTokenIs(E_RADIO)) {
        auto &radio = stream_.consumeToken(TokenType::Identifier);
        if (!allowNative) {
            throw CompilerError(radio.position(), "Native code is not allowed in this context.");
        }
        auto &indexToken = stream_.consumeToken(TokenType::Integer);
        auto index = std::stoll(indexToken.value().utf8(), nullptr, 0);
        if (index < 1 || index > UINT16_MAX) {
            throw CompilerError(indexToken.position(), "Linking Table Index is not in range.");
        }
        function->setLinkingTableIndex(static_cast<int>(index));
    }
    else {
        stream_.requireIdentifier(E_GRAPES);
        parseBodyBlock(function);
    }
}

void AbstractParser::parseBodyBlock(Function *function) {
    function->setTokenStream(stream_);

    int depth = 0;
    while (true) {
        auto &token = stream_.consumeToken();
        if (token.isIdentifier(E_GRAPES)) {
            depth++;
        }
        else if (token.isIdentifier(E_WATERMELON)) {
            if (depth == 0) {
                break;
            }
            depth--;
        }
    }
}
