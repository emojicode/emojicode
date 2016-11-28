//
//  PackageParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include "PackageParser.hpp"
#include "Class.hpp"
#include "Function.hpp"
#include "Enum.hpp"
#include "ValueType.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"

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

                package_->loadPackage(nameToken.value().utf8().c_str(), namespaceToken.value(), theToken);
                
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
                    throw CompilerErrorException(theToken, "You may not set 📻 for the _ package.");
                }
                continue;
            case E_CRYSTAL_BALL: {
                final.disallow();
                exported.disallow();
                if (package_->validVersion()) {
                    throw CompilerErrorException(theToken, "Package version already declared.");
                }
                
                auto major = std::stoi(stream_.consumeToken(TokenType::Integer).value().utf8());
                auto minor = std::stoi(stream_.consumeToken(TokenType::Integer).value().utf8());
                
                package_->setPackageVersion(PackageVersion(major, minor));
                if (!package_->validVersion()) {
                    throw CompilerErrorException(theToken, "The provided package version is not valid.");
                }
                
                package_->setDocumentation(documentation.get());
                
                continue;
            }
            case E_WALE: {
                final.disallow();
                exported.disallow();
                documentation.disallow();

                auto parsedType = parseTypeName();
                
                if (parsedType.optional) {
                    throw CompilerErrorException(parsedType.token, "Optional types are not extendable.");
                }
                
                Type type = typeNothingness;
                
                if (!package_->fetchRawType(parsedType, &type)) {
                    throw CompilerErrorException(parsedType.token, "Class does not exist.");
                }
                if (type.type() != TypeContent::Class && type.type() != TypeContent::ValueType) {
                    throw CompilerErrorException(parsedType.token, "Only classes and value types are extendable.");
                }
                
                // Native extensions are allowed if the class was defined in this package.
                parseTypeDefinitionBody(type, nullptr, type.eclass()->package() == package_);
                
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
                
                PackageParser(package_, lex(fileString.c_str())).parse();

                continue;
            }
            case E_CHEQUERED_FLAG: {
                final.disallow();
                if (Function::foundStart) {
                    throw CompilerErrorException(theToken, "Duplicate 🏁.");
                }
                Function::foundStart = true;
                
                auto function = new Function(EmojicodeString(E_CHEQUERED_FLAG), AccessLevel::Public, false,
                                             typeNothingness, package_, theToken, false, documentation.get(), false,
                                             CallableParserAndGeneratorMode::Function);
                parseReturnType(function, typeNothingness);
                if (function->returnType.type() != TypeContent::Nothingness &&
                    !function->returnType.compatibleTo(typeInteger, typeNothingness)) {
                    throw CompilerErrorException(theToken, "🏁 must either return ✨ or 🚂.");
                }
                parseBody(function, false);

                Function::start = function;
                break;
            }
            default:
                throw CompilerErrorException(theToken, "Unexpected identifier %s", theToken.value().utf8().c_str());
        }
    }
}

void PackageParser::reservedEmojis(const Token &token, const char *place) const {
    EmojicodeChar name = token.value()[0];
    switch (name) {
        case E_CUSTARD:
        case E_DOUGHNUT:
        case E_SHORTCAKE:
        case E_CHOCOLATE_BAR:
        case E_COOKING:
        case E_COOKIE:
        case E_LOLLIPOP:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_RED_APPLE:
        case E_BEER_MUG:
        case E_CLINKING_BEER_MUGS:
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
            throw CompilerErrorException(token, "%s is reserved and cannot be used as %s name.",
                                         token.value().utf8().c_str(), place);
        }
    }
}

//MARK: Utilities

ParsedTypeName PackageParser::parseAndValidateNewTypeName() {
    auto parsedTypeName = parseTypeName();
    
    if (parsedTypeName.optional) {
        throw CompilerErrorException(parsedTypeName.token, "🍬 cannot be declared as type.");
    }
    
    Type type = typeNothingness;
    if (package_->fetchRawType(parsedTypeName, &type)) {
        auto str = type.toString(typeNothingness, true);
        throw CompilerErrorException(parsedTypeName.token, "Type %s is already defined.", str.c_str());
    }
    
    return parsedTypeName;
}

void PackageParser::parseGenericArgumentList(TypeDefinitionFunctional *typeDef, TypeContext tc) {
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken(TokenType::Identifier);
        
        auto &variable = stream_.consumeToken(TokenType::Variable);
        auto constraintType = parseTypeDeclarative(tc, TypeDynamism::None, typeNothingness, nullptr, true);
        typeDef->addGenericArgument(variable, constraintType);
    }
}

static AccessLevel readAccessLevel(TokenStream *stream) {
    auto access = AccessLevel::Public;
    if (stream->nextTokenIs(E_CLOSED_LOCK_WITH_KEY)) {
        access = AccessLevel::Protected;
        stream->consumeToken(TokenType::Identifier);
    }
    else if (stream->nextTokenIs(E_LOCK)) {
        access = AccessLevel::Private;
        stream->consumeToken(TokenType::Identifier);
    }
    else if (stream->nextTokenIs(E_OPEN_LOCK)) {
        stream->consumeToken(TokenType::Identifier);
    }
    return access;
}

void PackageParser::parseProtocol(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto protocol = new Protocol(parsedTypeName.name, package_, theToken, documentation);
    
    parseGenericArgumentList(protocol, Type(protocol, false));
    protocol->finalizeGenericArguments();
    
    stream_.requireIdentifier(E_GRAPES);

    auto protocolType = Type(protocol, false);
    package_->registerType(protocolType, parsedTypeName.name, parsedTypeName.ns, exported);
    
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto &token = stream_.consumeToken(TokenType::Identifier);
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&stream_);
        
        if (!token.isIdentifier(E_PIG)) {
            throw CompilerErrorException(token, "Only method declarations are allowed inside a protocol.");
        }
        
        auto &methodName = stream_.consumeToken(TokenType::Identifier);
        
        auto method = new Function(methodName.value(), AccessLevel::Public, false, protocolType, package_,
                                 methodName.position(), false, documentation.get(), deprecated.set(),
                                 CallableParserAndGeneratorMode::ObjectMethod);
        auto a = parseArgumentList(method, protocolType);
        auto b = parseReturnType(method, protocolType);
        if (a || b) {
            protocol->setUsesSelf();
        }
        
        protocol->addMethod(method);
    }
    stream_.consumeToken(TokenType::Identifier);
}

void PackageParser::parseEnum(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    
    Enum *eenum = new Enum(parsedTypeName.name, package_, theToken, documentation);
    
    package_->registerType(Type(eenum, false), parsedTypeName.name, parsedTypeName.ns, exported);
    
    stream_.requireIdentifier(E_GRAPES);
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto &token = stream_.consumeToken(TokenType::Identifier);
        eenum->addValueFor(token.value(), token, documentation.get());
    }
    stream_.consumeToken(TokenType::Identifier);
}

void PackageParser::parseClass(const EmojicodeString &documentation, const Token &theToken, bool exported, bool final) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    
    auto eclass = new Class(parsedTypeName.name, package_, theToken, documentation, final);
    
    parseGenericArgumentList(eclass, Type(eclass));
    
    if (!stream_.nextTokenIs(E_GRAPES)) {
        auto parsedTypeName = parseTypeName();

        Type type = typeNothingness;
        if (!package_->fetchRawType(parsedTypeName, &type)) {
            throw CompilerErrorException(parsedTypeName.token, "Superclass type does not exist.");
        }
        if (type.type() != TypeContent::Class) {
            throw CompilerErrorException(parsedTypeName.token, "The superclass must be a class.");
        }
        if (type.optional()) {
            throw CompilerErrorException(parsedTypeName.token, "An 🍬 is not a valid superclass.");
        }
        
        eclass->setSuperclass(type.eclass());
        eclass->setSuperTypeDef(eclass->superclass());
        parseGenericArgumentsForType(&type, Type(eclass), TypeDynamism::GenericTypeVariables, parsedTypeName.token);
        eclass->setSuperGenericArguments(type.genericArguments);
        
        if (eclass->superclass()->final()) {
            auto string = type.toString(Type(eclass), true);
            throw CompilerErrorException(parsedTypeName.token,
                                         "%s can’t be used as superclass as it was marked with 🔏.",
                                         string.c_str());
        }
    }
    else {
        eclass->finalizeGenericArguments();
    }
    
    package_->registerType(Type(eclass), parsedTypeName.name, parsedTypeName.ns, exported);
    package_->registerClass(eclass);
    
    std::set<EmojicodeString> requiredInitializers;
    if (eclass->superclass() != nullptr) {
        // This set contains methods that must be implemented.
        // If a method is implemented it gets removed from this list by parseClassBody.
        requiredInitializers = std::set<EmojicodeString>(eclass->superclass()->requiredInitializers());
    }
    
    parseTypeDefinitionBody(Type(eclass), &requiredInitializers, true);
    
    if (requiredInitializers.size()) {
        throw CompilerErrorException(eclass->position(), "Required initializer %s was not implemented.",
                                     (*requiredInitializers.begin()).utf8().c_str());
    }
}

void PackageParser::parseValueType(const EmojicodeString &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    
    auto valueType = new ValueType(parsedTypeName.name, package_, theToken, documentation);
    
    auto valueTypeContent = Type(valueType, false);
    parseGenericArgumentList(valueType, valueTypeContent);
    valueType->finalizeGenericArguments();
    
    package_->registerType(valueTypeContent, parsedTypeName.name, parsedTypeName.ns, exported);
    parseTypeDefinitionBody(valueTypeContent, nullptr, true);
}

void PackageParser::parseTypeDefinitionBody(Type typed, std::set<EmojicodeString> *requiredInitializers,
                                            bool allowNative) {
    allowNative = allowNative && package_->requiresBinary();
    
    stream_.requireIdentifier(E_GRAPES);
    
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto deprecated = Attribute<E_WARNING_SIGN>().parse(&stream_);
        auto final = Attribute<E_LOCK_WITH_INK_PEN>().parse(&stream_);
        AccessLevel accessLevel = readAccessLevel(&stream_);
        auto override = Attribute<E_BLACK_NIB>().parse(&stream_);
        auto staticOnType = Attribute<E_RABBIT>().parse(&stream_);
        auto required = Attribute<E_KEY>().parse(&stream_);
        auto canReturnNothingness = Attribute<E_CANDY>().parse(&stream_);
        
        auto eclass = typed.type() == TypeContent::Class ? typed.eclass() : nullptr;
        
        auto &token = stream_.consumeToken(TokenType::Identifier);
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                canReturnNothingness.disallow();
                deprecated.disallow();
                documentation.disallow();
                
                auto &variableName = stream_.consumeToken(TokenType::Variable);
                auto type = parseTypeDeclarative(typed, TypeDynamism::GenericTypeVariables);
                typed.typeDefinitionFunctional()->addInstanceVariable(InstanceVariableDeclaration(variableName.value(),
                                                                                    type, variableName.position()));
                break;
            }
            case E_CROCODILE: {
                if (!eclass) {
                    throw CompilerErrorException(token, "🐊 not supported yet.");
                }
                
                staticOnType.disallow();
                override.disallow();
                final.disallow();
                required.disallow();
                canReturnNothingness.disallow();
                deprecated.disallow();
                documentation.disallow();
                
                Type type = parseTypeDeclarative(typed, TypeDynamism::GenericTypeVariables, typeNothingness, nullptr,
                                                 true);
                
                if (type.optional()) {
                    throw CompilerErrorException(token, "A class cannot conform to an 🍬 protocol.");
                }
                if (type.type() != TypeContent::Protocol) {
                    throw CompilerErrorException(token, "The given type is not a protocol.");
                }
                
                eclass->addProtocol(type);
                break;
            }
            case E_PIG: {
                required.disallow();
                canReturnNothingness.disallow();

                auto &methodName = stream_.consumeToken(TokenType::Identifier);
                
                if (staticOnType.set()) {
                    auto *classMethod = new Function(methodName.value(), accessLevel, final.set(), typed, package_,
                                                     token.position(), override.set(), documentation.get(),
                                                     deprecated.set(),
                                                     eclass ? CallableParserAndGeneratorMode::ClassMethod :
                                                        CallableParserAndGeneratorMode::Function);
                    auto context = TypeContext(typed, classMethod);
                    parseGenericArgumentsInDefinition(classMethod, context);
                    parseArgumentList(classMethod, context);
                    parseReturnType(classMethod, context);
                    parseBody(classMethod, allowNative);
                    
                    typed.typeDefinitionFunctional()->addTypeMethod(classMethod);
                }
                else {
                    reservedEmojis(methodName, "method");
                    
                    auto *method = new Function(methodName.value(), accessLevel, final.set(), typed,
                                                package_, token.position(), override.set(), documentation.get(),
                                                deprecated.set(),
                                                eclass ? CallableParserAndGeneratorMode::ObjectMethod :
                                                    CallableParserAndGeneratorMode::ThisContextFunction);
                    auto context = TypeContext(typed, method);
                    parseGenericArgumentsInDefinition(method, context);
                    parseArgumentList(method, context);
                    parseReturnType(method, context);
                    parseBody(method, allowNative);
                    
                    typed.typeDefinitionFunctional()->addMethod(method);
                }
                break;
            }
            case E_CAT: {
                staticOnType.disallow();
                override.disallow();
                
                EmojicodeString name = stream_.consumeToken(TokenType::Identifier).value();
                Initializer *initializer = new Initializer(name, accessLevel, final.set(), typed, package_,
                                                           token.position(), override.set(), documentation.get(),
                                                           deprecated.set(), required.set(),
                                                           canReturnNothingness.set(),
                                                           eclass ? CallableParserAndGeneratorMode::ObjectInitializer :
                                                              CallableParserAndGeneratorMode::ThisContextFunction);
                auto context = TypeContext(typed, initializer);
                parseGenericArgumentsInDefinition(initializer, context);
                parseArgumentList(initializer, context, true);
                parseBody(initializer, allowNative);
                
                if (requiredInitializers) {
                    requiredInitializers->erase(name);
                }
                
                typed.typeDefinitionFunctional()->addInitializer(initializer);
                break;
            }
            case E_FOG: {
                if (eclass) {
                    throw CompilerErrorException(token, "🌫 can only occur in a value type.");
                }
                typed.valueType()->makePrimitive();
                break;
            }
            default:
                throw CompilerErrorException(token, "Unexpected identifier %s.", token.value().utf8().c_str());
        }
    }
    stream_.consumeToken(TokenType::Identifier);
}
