//
//  PackageParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "PackageParser.hpp"
#include "../../utf8.h"
#include "../Function.hpp"
#include "../Initializer.hpp"
#include "../ProtocolFunction.hpp"
#include "../Types/Class.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeContext.hpp"
#include "../Types/ValueType.hpp"
#include "FunctionParser.hpp"
#include <cstring>
#include <experimental/optional>

namespace EmojicodeCompiler {

void PackageParser::parse() {
    while (stream_.hasMoreTokens()) {
        auto documentation = Documentation().parse(&stream_);
        auto exported = Attribute<E_EARTH_GLOBE_EUROPE_AFRICA>().parse(&stream_);
        auto final = Attribute<E_LOCK_WITH_INK_PEN>().parse(&stream_);
        auto &theToken = stream_.consumeToken(TokenType::Identifier);
        switch (theToken.value()[0]) {
            case E_PACKAGE: {
                exported.disallow();
                final.disallow();
                documentation.disallow();

                auto &nameToken = stream_.consumeToken(TokenType::Variable);
                auto &namespaceToken = stream_.consumeToken(TokenType::Identifier);

                package_->loadPackage(nameToken.value().utf8(), namespaceToken.value(), theToken.position());

                continue;
            }
            case E_CROCODILE:
                final.disallow();
                parseProtocol(documentation.get(), theToken, exported.set());
                continue;
            case E_TURKEY:
                final.disallow();
                parseEnum(documentation.get(), theToken, exported.set());
                continue;
            case E_RADIO:
                final.disallow();
                exported.disallow();
                documentation.disallow();
                package_->setRequiresBinary();
                if (package_->name() == "_") {
                    throw CompilerError(theToken.position(), "You may not set 📻 for the _ package.");
                }
                continue;
            case E_TRIANGLE_POINTED_DOWN: {
                TypeIdentifier alias = parseTypeIdentifier();
                Type type = Type::nothingness();
                package_->fetchRawType(parseTypeIdentifier(), false, &type);

                package_->registerType(type, alias.name, alias.ns, false, theToken.position());
                continue;
            }
            case E_CRYSTAL_BALL: {
                final.disallow();
                exported.disallow();
                if (package_->validVersion()) {
                    throw CompilerError(theToken.position(), "Package version already declared.");
                }

                auto major = std::stoi(stream_.consumeToken(TokenType::Integer).value().utf8());
                auto minor = std::stoi(stream_.consumeToken(TokenType::Integer).value().utf8());

                package_->setPackageVersion(PackageVersion(major, minor));
                if (!package_->validVersion()) {
                    throw CompilerError(theToken.position(), "The provided package version is not valid.");
                }

                package_->setDocumentation(documentation.get());

                continue;
            }
            case E_WALE: {
                final.disallow();
                exported.disallow();
                documentation.disallow();

                auto parsedType = parseTypeIdentifier();

                Type type = Type::nothingness();

                if (!package_->fetchRawType(parsedType, false, &type)) {
                    throw CompilerError(parsedType.token.position(), "Type does not exist.");
                }
                if (type.type() != TypeType::Class && type.type() != TypeType::ValueType) {
                    throw CompilerError(parsedType.token.position(), "Only classes and value types are extendable.");
                }

                auto extension = Extension(type, package_, theToken.position(), documentation.get());
                if (type.typeDefinition()->package() != package_) {
                    type = Type(&extension);
                }

                parseTypeDefinitionBody(type, nullptr);
                package_->registerExtension(extension);
                continue;
            }
            case E_RABBIT:
                parseClass(documentation.get(), theToken, exported.set(), final.set());
                continue;
            case E_DOVE_OF_PEACE:
                final.disallow();
                parseValueType(documentation.get(), theToken, exported.set());
                continue;
            case E_SCROLL: {
                final.disallow();
                exported.disallow();
                documentation.disallow();

                auto &pathString = stream_.consumeToken(TokenType::String);
                auto relativePath = std::string(pathString.position().file);
                auto fileString = pathString.value().utf8();

                auto lastSlash = relativePath.find_last_of('/');
                if (lastSlash != relativePath.npos) {
                    fileString = relativePath.substr(0, lastSlash) + "/" + fileString;
                }

                PackageParser(package_, Lexer::lexFile(fileString)).parse();

                continue;
            }
            case E_CHEQUERED_FLAG: {
                final.disallow();
                if (Function::foundStart) {
                    throw CompilerError(theToken.position(), "Duplicate 🏁.");
                }
                Function::foundStart = true;

                auto function = new Function(EmojicodeString(E_CHEQUERED_FLAG), AccessLevel::Public, false,
                                             Type::nothingness(), package_, theToken.position(), false,
                                             documentation.get(), false, false,
                                             FunctionType::Function);
                parseReturnType(function, Type::nothingness());
                if (function->returnType.type() != TypeType::Nothingness &&
                    !function->returnType.compatibleTo(Type::integer(), Type::nothingness())) {
                    throw CompilerError(theToken.position(), "🏁 must either return ✨ or 🚂.");
                }
                parseFunction(function);
                package_->registerFunction(function);

                Function::start = function;
                break;
            }
            default:
                throw CompilerError(theToken.position(), "Unexpected identifier %s", theToken.value().utf8().c_str());
        }
    }
}

void PackageParser::reservedEmojis(const Token &token, const char *place) const {
    EmojicodeChar name = token.value()[0];
    switch (name) {
        case E_CUSTARD:
        case E_DOUGHNUT:
        case E_SHORTCAKE:
        case E_COOKIE:
        case E_LOLLIPOP:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_RED_APPLE:
        case E_BEER_MUG:
        case E_LEMON:
        case E_GRAPES:
        case E_STRAWBERRY:
        case E_BLACK_SQUARE_BUTTON:
        case E_LARGE_BLUE_DIAMOND:
        case E_DOG:
        case E_HIGH_VOLTAGE_SIGN:
        case E_CLOUD:
        case E_BANANA:
        case E_HONEY_POT:
        case E_SOFT_ICE_CREAM:
        case E_ICE_CREAM:
        case E_TANGERINE:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_SPIRAL_SHELL:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            throw CompilerError(token.position(), "%s is reserved and cannot be used as %s name.",
                                token.value().utf8().c_str(), place);
        }
    }
}

TypeIdentifier PackageParser::parseAndValidateNewTypeName() {
    auto parsedTypeName = parseTypeIdentifier();

    Type type = Type::nothingness();
    if (package_->fetchRawType(parsedTypeName, false, &type)) {
        auto str = type.toString(Type::nothingness());
        throw CompilerError(parsedTypeName.token.position(), "Type %s is already defined.", str.c_str());
    }

    return parsedTypeName;
}

void PackageParser::parseGenericArgumentList(TypeDefinition *typeDef, const TypeContext& tc) {
    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        auto &variable = stream_.consumeToken(TokenType::Variable);
        auto constraint = parseType(tc, TypeDynamism::GenericTypeVariables);
        typeDef->addGenericArgument(variable.value(), constraint, variable.position());
    }
}

static AccessLevel readAccessLevel(TokenStream *stream) {
    auto access = AccessLevel::Public;
    if (stream->consumeTokenIf(E_CLOSED_LOCK_WITH_KEY)) {
        access = AccessLevel::Protected;
    }
    else if (stream->consumeTokenIf(E_LOCK)) {
        access = AccessLevel::Private;
    }
    else {
        stream->consumeTokenIf(E_OPEN_LOCK);
    }
    return access;
}

void PackageParser::parseProtocol(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto protocol = new Protocol(parsedTypeName.name, package_, theToken.position(), documentation);

    parseGenericArgumentList(protocol, Type(protocol, false));
    protocol->finalizeGenericArguments();

    stream_.requireIdentifier(E_GRAPES);

    auto protocolType = Type(protocol, false);
    package_->registerType(protocolType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto &token = stream_.consumeToken(TokenType::Identifier);
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&stream_);

        if (!token.isIdentifier(E_PIG)) {
            throw CompilerError(token.position(), "Only method declarations are allowed inside a protocol.");
        }

        auto &methodName = stream_.consumeToken(TokenType::Identifier, TokenType::Operator);

        auto method = new ProtocolFunction(methodName.value(), AccessLevel::Public, false, protocolType, package_,
                                           methodName.position(), false, documentation.get(), deprecated.set(), false,
                                           FunctionType::ObjectMethod);
        parseArgumentList(method, protocolType, false);
        parseReturnType(method, protocolType);

        protocol->addMethod(method);
    }
    stream_.consumeToken(TokenType::Identifier);
}

void PackageParser::parseEnum(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    Enum *eenum = new Enum(parsedTypeName.name, package_, theToken.position(), documentation);

    auto type = Type(eenum, false);
    package_->registerType(type, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    parseTypeDefinitionBody(type, nullptr);
    package_->registerValueType(eenum);
}

void PackageParser::parseClass(const EmojicodeString &documentation, const Token &theToken, bool exported, bool final) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto eclass = new Class(parsedTypeName.name, package_, theToken.position(), documentation, final);

    parseGenericArgumentList(eclass, Type(eclass, false));

    if (!stream_.nextTokenIs(E_GRAPES)) {
        auto classType = Type(eclass, false);  // New Type due to generic arguments now (partly) available.
        auto parsedTypeName = parseTypeIdentifier();

        Type type = Type::nothingness();
        if (!package_->fetchRawType(parsedTypeName, false, &type)) {
            throw CompilerError(parsedTypeName.token.position(), "Superclass type does not exist.");
        }
        if (type.type() != TypeType::Class) {
            throw CompilerError(parsedTypeName.token.position(), "The superclass must be a class.");
        }

        eclass->setSuperclass(type.eclass());
        eclass->setSuperTypeDef(eclass->superclass());
        parseGenericArgumentsForType(&type, classType, TypeDynamism::GenericTypeVariables,
                                     parsedTypeName.token.position());
        eclass->setSuperGenericArguments(type.genericArguments());

        if (eclass->superclass()->final()) {
            auto string = type.toString(classType);
            throw CompilerError(parsedTypeName.token.position(),
                                "%s can’t be used as superclass as it was marked with 🔏.", string.c_str());
        }
    }
    else {
        eclass->finalizeGenericArguments();
    }

    auto classType = Type(eclass, false);  // New Type due to generic arguments now available.
    package_->registerType(classType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    package_->registerClass(eclass);

    std::set<EmojicodeString> requiredInitializers;
    if (eclass->superclass() != nullptr) {
        // This set contains methods that must be implemented.
        // If a method is implemented it gets removed from this list by parseClassBody.
        requiredInitializers = std::set<EmojicodeString>(eclass->superclass()->requiredInitializers());
    }

    parseTypeDefinitionBody(classType, &requiredInitializers);

    if (!requiredInitializers.empty()) {
        throw CompilerError(eclass->position(), "Required initializer %s was not implemented.",
                                     (*requiredInitializers.begin()).utf8().c_str());
    }
}

void PackageParser::parseValueType(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto valueType = new ValueType(parsedTypeName.name, package_, theToken.position(), documentation);

    if (stream_.consumeTokenIf(E_WHITE_CIRCLE)) {
        valueType->makePrimitive();
    }

    parseGenericArgumentList(valueType, Type(valueType, false));
    valueType->finalizeGenericArguments();
    auto valueTypeContent = Type(valueType, false);

    package_->registerType(valueTypeContent, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    package_->registerValueType(valueType);
    parseTypeDefinitionBody(valueTypeContent, nullptr);
}

void PackageParser::parseTypeDefinitionBody(const Type &typed, std::set<EmojicodeString> *requiredInitializers) {
    stream_.requireIdentifier(E_GRAPES);

    auto owningType = typed;
    if (typed.type() == TypeType::Extension) {
        owningType = dynamic_cast<Extension *>(typed.typeDefinition())->extendedType();
    }

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&stream_);
        auto final = Attribute<E_LOCK_WITH_INK_PEN>().parse(&stream_);
        AccessLevel accessLevel = readAccessLevel(&stream_);
        auto override = Attribute<E_BLACK_NIB>().parse(&stream_);
        auto staticOnType = Attribute<E_RABBIT>().parse(&stream_);
        auto mutating = Attribute<E_CRAYON>().parse(&stream_);
        auto required = Attribute<E_KEY>().parse(&stream_);

        auto &token = stream_.consumeToken(TokenType::Identifier);
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                deprecated.disallow();
                documentation.disallow();
                mutating.disallow();

                if (typed.type() == TypeType::Enum) {
                    throw CompilerError(token.position(), "Enums cannot have instance variable.");
                }

                auto &variableName = stream_.consumeToken(TokenType::Variable);
                auto type = parseType(typed, TypeDynamism::GenericTypeVariables);
                auto instanceVar = InstanceVariableDeclaration(variableName.value(), type, variableName.position());
                typed.typeDefinition()->addInstanceVariable(instanceVar);
                break;
            }
            case E_RADIO_BUTTON: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                deprecated.disallow();
                mutating.disallow();

                auto &token = stream_.consumeToken(TokenType::Identifier);
                typed.eenum()->addValueFor(token.value(), token.position(), documentation.get());
                break;
            }
            case E_CROCODILE: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                deprecated.disallow();
                documentation.disallow();
                mutating.disallow();

                Type type = parseType(typed, TypeDynamism::GenericTypeVariables);

                if (type.type() != TypeType::Protocol || type.optional()) {
                    throw CompilerError(token.position(), "The given type is not a protocol.");
                }

                typed.typeDefinition()->addProtocol(type, token.position());
                break;
            }
            case E_PIG: {
                required.disallow();
                if (typed.type() != TypeType::Class) {
                    override.disallow();
                    final.disallow();
                }

                auto &methodName = stream_.consumeToken(TokenType::Identifier, TokenType::Operator);

                if (staticOnType.set()) {
                    mutating.disallow();
                    auto *typeMethod = new Function(methodName.value(), accessLevel, final.set(), owningType, package_,
                                                    token.position(), override.set(), documentation.get(),
                                                    deprecated.set(), true, typed.type() == TypeType::Class ?
                                                    FunctionType::ClassMethod :
                                                    FunctionType::Function);
                    auto context = TypeContext(typed, typeMethod);
                    parseGenericArgumentsInDefinition(typeMethod, context);
                    parseArgumentList(typeMethod, context);
                    parseReturnType(typeMethod, context);
                    parseFunction(typeMethod);

                    typed.typeDefinition()->addTypeMethod(typeMethod);
                }
                else {
                    auto isMutating = true;
                    if (typed.type() == TypeType::ValueType) {
                        isMutating = mutating.set();
                    }
                    else {
                        mutating.disallow();
                    }
                    reservedEmojis(methodName, "method");

                    auto *method = new Function(methodName.value(), accessLevel, final.set(), owningType,
                                                package_, token.position(), override.set(), documentation.get(),
                                                deprecated.set(), isMutating, typed.type() == TypeType::Class ?
                                                FunctionType::ObjectMethod :
                                                FunctionType::ValueTypeMethod);
                    auto context = TypeContext(typed, method);
                    parseGenericArgumentsInDefinition(method, context);
                    parseArgumentList(method, context);
                    parseReturnType(method, context);
                    parseFunction(method);

                    typed.typeDefinition()->addMethod(method);
                }
                break;
            }
            case E_CAT: {
                if (typed.type() == TypeType::Enum) {
                    throw CompilerError(token.position(), "Enums cannot have custom initializers.");
                }

                std::experimental::optional<Type> errorType = std::experimental::nullopt;
                if (stream_.nextTokenIs(E_POLICE_CARS_LIGHT)) {
                    if (typed.type() != TypeType::Class) {
                        throw CompilerError(token.position(), "Only classes can have error-prone initializers.");
                    }
                    auto &token = stream_.consumeToken(TokenType::Identifier);
                    errorType = parseErrorEnumType(typed, TypeDynamism::None, token.position());
                }

                staticOnType.disallow();
                override.disallow();
                mutating.disallow();
                if (typed.type() != TypeType::Class) {
                    required.disallow();
                }

                EmojicodeString name = stream_.consumeToken(TokenType::Identifier).value();
                Initializer *initializer = new Initializer(name, accessLevel, final.set(), owningType, package_,
                                                           token.position(), override.set(), documentation.get(),
                                                           deprecated.set(), required.set(), errorType,
                                                           typed.type() == TypeType::Class ?
                                                           FunctionType::ObjectInitializer :
                                                           FunctionType::ValueTypeInitializer);
                auto context = TypeContext(typed, initializer);
                parseGenericArgumentsInDefinition(initializer, context);
                parseArgumentList(initializer, context, true);
                parseFunction(initializer);

                if (requiredInitializers != nullptr) {
                    requiredInitializers->erase(name);
                }

                typed.typeDefinition()->addInitializer(initializer);
                break;
            }
            default:
                throw CompilerError(token.position(), "Unexpected identifier %s.", token.value().utf8().c_str());
        }
    }
    stream_.consumeToken(TokenType::Identifier);
}

void PackageParser::parseFunction(Function *function) {
    if (stream_.consumeTokenIf(E_RADIO)) {
        auto &indexToken = stream_.consumeToken(TokenType::Integer);
        auto index = std::stoll(indexToken.value().utf8(), nullptr, 0);
        if (index < 1 || index > UINT16_MAX) {
            throw CompilerError(indexToken.position(), "Linking Table Index is not in range.");
        }
        function->setLinkingTableIndex(static_cast<int>(index));
    }
    else {
        stream_.requireIdentifier(E_GRAPES);
        try {
            auto ast = FunctionParser(package_, stream_, function->typeContext()).parse();
            function->setAst(ast);
        }
        catch (CompilerError &ce) {
            printError(ce);
        }
    }
}

}  // namespace EmojicodeCompiler
