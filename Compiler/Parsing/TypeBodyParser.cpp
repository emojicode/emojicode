//
//  TypeBodyParser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "TypeBodyParser.hpp"
#include "FunctionParser.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Types/Enum.hpp"

namespace EmojicodeCompiler {

void TypeBodyParser::parseFunctionBody(Function *function) {
    if (stream_.consumeTokenIf(E_RADIO)) {
        auto token = stream_.consumeToken(TokenType::String);
        if (token.value().empty()) {
            throw CompilerError(token.position(), "The external name must not be empty.");
        }
        function->setExternalName(utf8(token.value()));
        return;
    }

    if (interface_) {
        function->makeExternal();
        return;
    }

    stream_.consumeToken(TokenType::BlockBegin);
    try {
        auto ast = factorFunctionParser(package_, stream_, function->typeContext(), function)->parse();
        function->setAst(ast);
    }
    catch (CompilerError &ce) {
        package_->compiler()->error(ce);
    }
}

void TypeBodyParser::parseFunction(Function *function, bool inititalizer) {
    auto context = TypeContext(owningType(), function);
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
    Type type = parseType(TypeContext(owningType()));

    if (type.type() != TypeType::Protocol) {
        package_->compiler()->error(CompilerError(p, "The given type is not a protocol."));
        return;
    }

    type_.typeDefinition()->addProtocol(type, p);
}

void TypeBodyParser::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    throw CompilerError(p, "Enum values can only be declared in enums.");
}

void EnumTypeBodyParser::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    auto token = stream_.consumeToken(TokenType::Identifier);
    owningType().eenum()->addValueFor(token.value(), token.position(), documentation.get());
}

void EnumTypeBodyParser::parseInstanceVariable(const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have instance variable.");
}

Initializer* EnumTypeBodyParser::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                  const Documentation &documentation, AccessLevel access,
                                                  const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have custom initializers.");
}

void ClassTypeBodyParser::parse() {
    TypeBodyParser::parse();
    for (auto init : requiredInitializers_) {
        package_->compiler()->error(CompilerError(owningType().typeDefinition()->position(), "Required initializer ",
                                             utf8(init), " was not implemented."));
    }
}

void ClassTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                      const Documentation &documentation, AccessLevel access, bool imperative,
                                      const SourcePosition &p) {
    TypeBodyParser::parseMethod(name, attributes.allow(Attribute::Final).allow(Attribute::Override), documentation,
                                access, imperative, p);
}

void ValueTypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                      const Documentation &documentation, AccessLevel access, bool imperative,
                                      const SourcePosition &p) {
    if (!attributes.has(Attribute::StaticOnType)) {
        attributes.allow(Attribute::Mutating);
    }
    TypeBodyParser::parseMethod(name, attributes, documentation, access, imperative, p);
}

Initializer* ClassTypeBodyParser::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                   const Documentation &documentation, AccessLevel access,
                                                   const SourcePosition &p) {
    auto init = TypeBodyParser::parseInitializer(name, attributes.allow(Attribute::Required), documentation, access, p);
    requiredInitializers_.erase(init->name());
    return init;
}

void TypeBodyParser::parseInstanceVariable(const SourcePosition &p) {
    auto variableName = stream_.consumeToken(TokenType::Variable);
    auto type = parseType(TypeContext(type_));
    auto instanceVar = InstanceVariableDeclaration(variableName.value(), type, variableName.position());
    type_.typeDefinition()->addInstanceVariable(instanceVar);
}

void TypeBodyParser::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                 const Documentation &documentation, AccessLevel access, bool imperative,
                                 const SourcePosition &p) {
    attributes.allow(Attribute::Deprecated).allow(Attribute::StaticOnType).check(p, package_->compiler());

    if (attributes.has(Attribute::StaticOnType)) {
        auto typeMethod = std::make_unique<Function>(name, access, attributes.has(Attribute::Final), owningType(),
                                                     package_, p, attributes.has(Attribute::Override),
                                                     documentation.get(), attributes.has(Attribute::Deprecated),
                                                     true, imperative, type_.type() == TypeType::Class ?
                                                     FunctionType::ClassMethod : FunctionType::Function);
        parseFunction(typeMethod.get(), false);
        type_.typeDefinition()->addTypeMethod(std::move(typeMethod));
    }
    else {
        auto mutating = owningType().type() == TypeType::ValueType ? attributes.has(Attribute::Mutating) : true;
        auto method = std::make_unique<Function>(name, access, attributes.has(Attribute::Final), owningType(),
                                                 package_, p, attributes.has(Attribute::Override), documentation.get(),
                                                 attributes.has(Attribute::Deprecated), mutating, imperative,
                                                 type_.type() == TypeType::Class ? FunctionType::ObjectMethod :
                                                 FunctionType::ValueTypeMethod);
        parseFunction(method.get(), false);
        type_.typeDefinition()->addMethod(std::move(method));
    }
}

Initializer* TypeBodyParser::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                              const Documentation &documentation, AccessLevel access,
                                              const SourcePosition &p) {
    attributes.check(p, package_->compiler());

    Type errorType = Type::noReturn();
    if (stream_.nextTokenIs(TokenType::Error)) {
        if (type_.type() != TypeType::Class) {
            throw CompilerError(p, "Only classes can have error-prone initializers.");
        }
        auto token = stream_.consumeToken(TokenType::Error);
        errorType = parseErrorEnumType(TypeContext(type_), token.position());
    }


    auto initializer = std::make_unique<Initializer>(name, access, attributes.has(Attribute::Final), owningType(),
                                                     package_, p, attributes.has(Attribute::Override),
                                                     documentation.get(), attributes.has(Attribute::Deprecated),
                                                     attributes.has(Attribute::Required), errorType,
                                                     type_.type() == TypeType::Class ? FunctionType::ObjectInitializer :
                                                     FunctionType::ValueTypeInitializer);
    parseFunction(initializer.get(), true);
    return type_.typeDefinition()->addInitializer(std::move(initializer));
}

void TypeBodyParser::parse() {
    stream_.consumeToken(TokenType::BlockBegin);
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd)) {
        auto documentation = Documentation().parse(&stream_);
        auto attributes = TypeBodyAttributeParser().parse(&stream_);
        AccessLevel accessLevel = readAccessLevel();

        auto token = stream_.consumeToken();
        switch (token.type()) {
            case TokenType::EndInterrogativeArgumentList:
            case TokenType::EndArgumentList: {
                auto methodName = stream_.consumeToken(TokenType::Identifier);
                parseMethod(methodName.value(), attributes, documentation, accessLevel,
                            token.type() == TokenType::EndArgumentList, token.position());
                break;
            }
            case TokenType::Operator:
                parseMethod(token.value(), attributes, documentation, accessLevel, true, token.position());
                break;
            case TokenType::New: {
                if (attributes.has(Attribute::Mutating)) {
                    attributes.allow(Attribute::Mutating).check(token.position(), package_->compiler());
                    documentation.disallow();
                    parseInstanceVariable(token.position());
                    break;
                }

                std::u32string name = std::u32string(1, E_NEW_SIGN);
                if (stream_.nextTokenIs(TokenType::Identifier) && !stream_.nextTokenIs(E_RADIO)
                    && !stream_.nextTokenIs(E_BABY_BOTTLE)) {
                    name = stream_.consumeToken(TokenType::Identifier).value();
                }
                parseInitializer(name, attributes, documentation, accessLevel, token.position());
                break;
            }
            case TokenType::Identifier:
                switch (token.value().front()) {
                    case E_RADIO_BUTTON:
                        attributes.check(token.position(), package_->compiler());
                        parseEnumValue(token.position(), documentation);
                        break;
                    case E_CROCODILE:
                        attributes.check(token.position(), package_->compiler());
                        documentation.disallow();
                        parseProtocolConformance(token.position());
                        break;
                    default:
                        break;
                }
                break;
            default:
                throw CompilerError(token.position(), "Unexpected token ", token.stringName());
        }
    }
    stream_.consumeToken(TokenType::BlockEnd);
}

TypeBodyParser::~TypeBodyParser() = default;

}  // namespace EmojicodeCompiler
