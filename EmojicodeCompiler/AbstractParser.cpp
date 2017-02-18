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

ParsedTypeName AbstractParser::parseTypeName() {
    if (stream_.nextTokenIs(TokenType::Variable)) {
        throw CompilerError(stream_.consumeToken(TokenType::Variable), "Generic variables not allowed here.");
    }

    bool optional = false;
    if (stream_.consumeTokenIf(E_CANDY)) {
        optional = true;
    }

    EmojicodeString enamespace;

    if (stream_.consumeTokenIf(E_ORANGE_TRIANGLE)) {
        enamespace = stream_.consumeToken(TokenType::Identifier).value();
    }
    else {
        enamespace = globalNamespace;
    }

    auto &typeName = stream_.consumeToken(TokenType::Identifier);
    return ParsedTypeName(typeName.value(), enamespace, optional, typeName);
}

Type AbstractParser::parseTypeDeclarative(TypeContext ct, TypeDynamism dynamism, Type expectation,
                                          TypeDynamism *dynamicType, bool allowProtocolsUsingSelf) {
    if (stream_.nextTokenIs(E_MEDIUM_BLACK_CIRCLE)) {
        if (expectation.type() == TypeContent::Nothingness) {
            throw CompilerError(stream_.consumeToken(), "Cannot infer ⚫️.");
        }
        stream_.consumeToken();
        return expectation.copyWithoutOptional();
    }
    if (stream_.nextTokenIs(E_WHITE_SQUARE_BUTTON)) {
        auto &token = stream_.consumeToken();
        Type type = parseTypeDeclarative(ct, dynamism, expectation, dynamicType, allowProtocolsUsingSelf);
        if (!type.allowsMetaType() || type.meta()) {
            throw CompilerError(token, "Meta type of %s is restricted.", type.toString(ct, true).c_str());
        }
        type.setMeta(true);
        return type;
    }

    bool optional = false;
    if (stream_.consumeTokenIf(E_CANDY)) {
        optional = true;
    }

    if ((dynamism & TypeDynamism::GenericTypeVariables) != TypeDynamism::None &&
        (ct.calleeType().canHaveGenericArguments() || ct.function()) &&
        stream_.nextTokenIs(TokenType::Variable)) {
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

        throw CompilerError(variableToken, "No such generic type variable \"%s\".",
                                     variableToken.value().utf8().c_str());
    }
    else if (stream_.nextTokenIs(E_DOG)) {
        auto &ratToken = stream_.consumeToken(TokenType::Identifier);
        if ((dynamism & TypeDynamism::Self) == TypeDynamism::None) {
            throw CompilerError(ratToken, "🐕 not allowed here.");
        }
        if (dynamicType) *dynamicType = TypeDynamism::Self;
        return Type::self(optional);
    }
    else if (stream_.consumeTokenIf(E_GRAPES)) {
        if (dynamicType) *dynamicType = TypeDynamism::None;

        Type t = Type::callableIncomplete(optional);
        t.genericArguments.push_back(Type::nothingness());

        while (stream_.nextTokenIsEverythingBut(E_WATERMELON) && stream_.nextTokenIsEverythingBut(E_RIGHTWARDS_ARROW)) {
            t.genericArguments.push_back(parseTypeDeclarative(ct, dynamism));
        }

        if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW)) {
            t.genericArguments[0] = parseTypeDeclarative(ct, dynamism);
        }

        stream_.requireIdentifier(E_WATERMELON);
        return t;
    }
    else {
        if (dynamicType) *dynamicType = TypeDynamism::None;
        auto parsedType = parseTypeName();

        parsedType.optional = optional;

        auto type = Type::nothingness();
        if (!package_->fetchRawType(parsedType, &type)) {
            throw CompilerError(parsedType.token, "Could not find type %s in enamespace %s.",
                                         parsedType.name.utf8().c_str(), parsedType.ns.utf8().c_str());
        }

        parseGenericArgumentsForType(&type, ct, dynamism, parsedType.token);

        if (!allowProtocolsUsingSelf && type.type() == TypeContent::Protocol && type.protocol()->usesSelf()) {
            auto typeStr = type.toString(ct, true);
            throw CompilerError(parsedType.token,
                                         "Protocol %s can only be used as a generic constraint because it uses 🐓.",
                                         typeStr.c_str());
        }

        return type;
    }
}

void AbstractParser::parseGenericArgumentsForType(Type *type, TypeContext ct, TypeDynamism dynamism,
                                                  const Token& errorToken) {
    if (type->canHaveGenericArguments()) {
        auto typeDef = type->typeDefinitionFunctional();
        auto offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        type->genericArguments = std::vector<Type>(typeDef->superGenericArguments());

        if (typeDef->numberOfOwnGenericArguments()) {
            int count = 0;
            while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
                auto &token = stream_.consumeToken(TokenType::Identifier);

                Type ta = parseTypeDeclarative(ct, dynamism, Type::nothingness(), nullptr);

                auto i = count + offset;
                if (typeDef->numberOfGenericArgumentsWithSuperArguments() <= i) {
                    auto name = type->toString(ct, true);
                    throw CompilerError(token, "Too many generic arguments provided for %s.", name.c_str());
                }
                if (!ta.compatibleTo(typeDef->genericArgumentConstraints()[i], ct)) {
                    auto thisName = typeDef->genericArgumentConstraints()[i].toString(ct, true);
                    auto thatName = ta.toString(ct, true);
                    throw CompilerError(token, "Generic argument for %s is not compatible to constraint %s.",
                                                 thatName.c_str(), thisName.c_str());
                }

                type->genericArguments.push_back(ta);

                count++;
            }

            if (count != typeDef->numberOfOwnGenericArguments()) {
                auto str = type->toString(Type::nothingness(), true);
                throw CompilerError(errorToken, "Type %s requires %d generic arguments, but %d were given.",
                                             str.c_str(), typeDef->numberOfOwnGenericArguments(), count);
            }
        }
    }
}

bool AbstractParser::parseArgumentList(Callable *c, TypeContext ct, bool initializer) {
    bool usedSelf = false;
    bool argumentToVariable;

    while ((argumentToVariable = stream_.nextTokenIs(E_BABY_BOTTLE)) || stream_.nextTokenIs(TokenType::Variable)) {
        if (argumentToVariable) {
            auto &token = stream_.consumeToken(TokenType::Identifier);
            if (!initializer) {
                throw CompilerError(token, "🍼 can only be used with initializers.");
            }
        }

        TypeDynamism dynamism;
        auto &variableToken = stream_.consumeToken(TokenType::Variable);
        auto type = parseTypeDeclarative(ct, TypeDynamism::AllKinds, Type::nothingness(), &dynamism);

        c->arguments.push_back(Argument(variableToken, type));

        if (dynamism == TypeDynamism::Self) {
            usedSelf = true;
        }
        if (argumentToVariable) {
            static_cast<Initializer *>(c)->addArgumentToVariable(variableToken.value());
        }
    }

    if (c->arguments.size() > UINT8_MAX) {
        throw CompilerError(c->position(), "A function cannot take more than 255 arguments.");
    }
    return usedSelf;
}

bool AbstractParser::parseReturnType(Callable *c, TypeContext ct) {
    bool usedSelf = false;
    if (stream_.consumeTokenIf(E_RIGHTWARDS_ARROW)) {
        TypeDynamism dynamism;

        c->returnType = parseTypeDeclarative(ct, TypeDynamism::AllKinds, Type::nothingness(), &dynamism);
        if (dynamism == TypeDynamism::Self) {
            usedSelf = true;
        }
    }
    return usedSelf;
}

void AbstractParser::parseGenericArgumentsInDefinition(Function *function, TypeContext ct) {
    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        auto &variable = stream_.consumeToken(TokenType::Variable);

        Type t = parseTypeDeclarative(function->owningType(), TypeDynamism::GenericTypeVariables, Type::nothingness(),
                                      nullptr, true);
        function->genericArgumentConstraints.push_back(t);

        Type rType = Type(TypeContent::LocalReference, false,
                          static_cast<int>(function->genericArgumentVariables.size()), function);

        if (function->genericArgumentVariables.count(variable.value())) {
            throw CompilerError(variable, "A generic argument variable with the same name is already in use.");
        }
        function->genericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable.value(), rType));
    }
}

void AbstractParser::parseBody(Function *function, bool allowNative) {
    if (stream_.nextTokenIs(E_RADIO)) {
        auto &radio = stream_.consumeToken(TokenType::Identifier);
        if (!allowNative) {
            throw CompilerError(radio, "Native code is not allowed in this context.");
        }
        auto &indexToken = stream_.consumeToken(TokenType::Integer);
        auto index = std::stoll(indexToken.value().utf8(), 0, 0);
        if (index < 1 || index > UINT16_MAX) {
            throw CompilerError(indexToken.position(), "Linking Table Index is not in range.");
        }
        function->setLinkingTableIndex(static_cast<int>(index));
    }
    else {
        stream_.requireIdentifier(E_GRAPES);
        parseBody(function);
    }
}

void AbstractParser::parseBody(Callable *p) {
    p->setTokenStream(stream_);

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
