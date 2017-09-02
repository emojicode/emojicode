//
//  TypeBodyParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "TypeBodyParser.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Enum.hpp"
#include "FunctionParser.hpp"

namespace EmojicodeCompiler {

void TypeBodyParser::parseFunctionBody(Function *function) {
    if (stream_.consumeTokenIf(E_RADIO)) {
        auto &indexToken = stream_.consumeToken(TokenType::Integer);
        auto index = std::stoll(utf8(indexToken.value()), nullptr, 0);
        if (index < 1 || index > UINT16_MAX) {
            throw CompilerError(indexToken.position(), "Linking Table Index is not in range.");
        }
        function->setLinkingTableIndex(static_cast<int>(index));
    }
    else {
        stream_.requireIdentifier(E_GRAPES);
        try {
            auto ast = factorFunctionParser(package_, stream_, function->typeContext(), function)->parse();
            function->setAst(ast);
        }
        catch (CompilerError &ce) {
            package_->app()->error(ce);
        }
    }
}

void TypeBodyParser::parseFunction(Function *function, bool inititalizer) {
    auto context = TypeContext(type_, function);
    parseGenericParameters(function, context);
    parseParameters(function, context, inititalizer);
    if (!inititalizer) {
        parseReturnType(function, context);
    }
    parseFunctionBody(function);
}

AccessLevel TypeBodyParser::readAccessLevel() {
    auto access = AccessLevel::Public;
    if (stream_.consumeTokenIf(E_CLOSED_LOCK_WITH_KEY)) {
        access = AccessLevel::Protected;
    }
    else if (stream_.consumeTokenIf(E_LOCK)) {
        access = AccessLevel::Private;
    }
    else {
        stream_.consumeTokenIf(E_OPEN_LOCK);
    }
    return access;
}

void TypeBodyParser::parseProtocolConformance(const SourcePosition &p) {
    Type type = parseType(TypeContext(type_), TypeDynamism::GenericTypeVariables);

    if (type.type() != TypeType::Protocol || type.optional()) {
        package_->app()->error(CompilerError(p, "The given type is not a protocol."));
        return;
    }

    type_.typeDefinition()->addProtocol(type, p);
}

void TypeBodyParser::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    throw CompilerError(p, "Enum values can only be declared in enums.");
}

void EnumTypeBodyParser::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    auto &token = stream_.consumeToken(TokenType::Identifier);
    type_.eenum()->addValueFor(token.value(), token.position(), documentation.get());
}

void EnumTypeBodyParser::parseInstanceVariable(const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have instance variable.");
}

Initializer* EnumTypeBodyParser::parseInitializer(TypeBodyAttributeParser attributes, const Documentation &documentation,
                                                  AccessLevel access, const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have custom initializers.");
}

void ClassTypeBodyParser::parse() {
    TypeBodyParser::parse();
    for (auto &init : requiredInitializers_) {
        package_->app()->error(CompilerError(type_.typeDefinition()->position(), "Required initializer ",
                                             utf8(init), " was not implemented."));
    }
}

void ClassTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                      const Documentation &documentation, AccessLevel access, const SourcePosition &p) {
    TypeBodyParser::parseMethod(name, attributes.allow(Attribute::Final).allow(Attribute::Override), documentation,
                                access, p);
}

void ValueTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                      const Documentation &documentation, AccessLevel access, const SourcePosition &p) {
    if (!attributes.has(Attribute::StaticOnType)) {
        attributes.allow(Attribute::Mutating);
    }
    TypeBodyParser::parseMethod(name, attributes, documentation, access, p);
}

Initializer* ClassTypeBodyParser::parseInitializer(TypeBodyAttributeParser attributes,
                                                   const Documentation &documentation, AccessLevel access,
                                                   const SourcePosition &p) {
    auto init = TypeBodyParser::parseInitializer(attributes.allow(Attribute::Required), documentation, access, p);
    requiredInitializers_.erase(init->name());
    return init;
}

void TypeBodyParser::parseInstanceVariable(const SourcePosition &p) {
    auto &variableName = stream_.consumeToken(TokenType::Variable);
    auto type = parseType(TypeContext(type_), TypeDynamism::GenericTypeVariables);
    auto instanceVar = InstanceVariableDeclaration(variableName.value(), type, variableName.position());
    type_.typeDefinition()->addInstanceVariable(instanceVar);
}

void TypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                 const Documentation &documentation, AccessLevel access, const SourcePosition &p) {
    attributes.allow(Attribute::Deprecated).allow(Attribute::StaticOnType).check(p, package_->app());

    if (attributes.has(Attribute::StaticOnType)) {
        auto *typeMethod = new Function(name, access, attributes.has(Attribute::Final), owningType(),
                                        package_, p, attributes.has(Attribute::Override), documentation.get(),
                                        attributes.has(Attribute::Deprecated), true, type_.type() == TypeType::Class ?
                                        FunctionType::ClassMethod : FunctionType::Function);
        parseFunction(typeMethod, false);
        type_.typeDefinition()->addTypeMethod(typeMethod);
    }
    else {
        auto mutating = type_.type() == TypeType::ValueType ? attributes.has(Attribute::Mutating) : true;
        auto *method = new Function(name, access, attributes.has(Attribute::Final), owningType(),
                                    package_, p, attributes.has(Attribute::Override), documentation.get(),
                                    attributes.has(Attribute::Deprecated), mutating,
                                    type_.type() == TypeType::Class ? FunctionType::ObjectMethod :
                                    FunctionType::ValueTypeMethod);
        parseFunction(method, false);
        type_.typeDefinition()->addMethod(method);
    }
}

Initializer* TypeBodyParser::parseInitializer(TypeBodyAttributeParser attributes, const Documentation &documentation,
                                              AccessLevel access, const SourcePosition &p) {
    attributes.check(p, package_->app());

    std::experimental::optional<Type> errorType = std::experimental::nullopt;
    if (stream_.nextTokenIs(E_POLICE_CARS_LIGHT)) {
        if (type_.type() != TypeType::Class) {
            throw CompilerError(p, "Only classes can have error-prone initializers.");
        }
        auto &token = stream_.consumeToken(TokenType::Identifier);
        errorType = parseErrorEnumType(TypeContext(type_), TypeDynamism::None, token.position());
    }

    std::u32string name = stream_.consumeToken(TokenType::Identifier).value();
    auto initializer = new Initializer(name, access, attributes.has(Attribute::Final), owningType(),
                                       package_, p, attributes.has(Attribute::Override), documentation.get(),
                                       attributes.has(Attribute::Deprecated), attributes.has(Attribute::Required),
                                       errorType, type_.type() == TypeType::Class ? FunctionType::ObjectInitializer :
                                       FunctionType::ValueTypeInitializer);
    parseFunction(initializer, true);
    type_.typeDefinition()->addInitializer(initializer);
    return initializer;
}

void TypeBodyParser::parse() {
    stream_.requireIdentifier(E_GRAPES);

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto attributes = TypeBodyAttributeParser().parse(&stream_);
        AccessLevel accessLevel = readAccessLevel();

        auto &token = stream_.consumeToken();
        switch (token.type()) {
            case TokenType::EndArgumentList: {
                auto &methodName = stream_.consumeToken(TokenType::Identifier);
                parseMethod(methodName.value(), attributes, documentation, accessLevel, token.position());
                break;
            }
            case TokenType::Operator: {
                parseMethod(token.value(), attributes, documentation, accessLevel, token.position());
                break;
            }
            case TokenType::Identifier:
                switch (token.value().front()) {
                    case E_SHORTCAKE:
                        attributes.check(token.position(), package_->app());
                        documentation.disallow();
                        parseInstanceVariable(token.position());
                        break;
                    case E_RADIO_BUTTON:
                        attributes.check(token.position(), package_->app());
                        parseEnumValue(token.position(), documentation);
                        break;
                    case E_CROCODILE:
                        attributes.check(token.position(), package_->app());
                        documentation.disallow();
                        parseProtocolConformance(token.position());
                        break;
                    case E_CAT:
                        parseInitializer(attributes, documentation, accessLevel, token.position());
                        break;
                    default:
                        break;
                }
                break;
            default:
                throw CompilerError(token.position(), "Unexpected token ", token.stringName());
        }
    }
    stream_.consumeToken(TokenType::Identifier);
}

TypeBodyParser::~TypeBodyParser() = default;

}  // namespace EmojicodeCompiler
