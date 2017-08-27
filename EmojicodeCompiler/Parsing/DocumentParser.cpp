//
//  DocumentParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "DocumentParser.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"
#include "../Functions/ProtocolFunction.hpp"
#include "../Types/Class.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeContext.hpp"
#include "../Types/ValueType.hpp"
#include "FunctionParser.hpp"
#include "TypeBodyParser.hpp"
#include <cstring>
#include <experimental/optional>

namespace EmojicodeCompiler {

void DocumentParser::parse() {
    while (stream_.hasMoreTokens()) {
        auto documentation = Documentation().parse(&stream_);
        auto attributes = PackageAttributeParser().parse(&stream_);
        auto &theToken = stream_.consumeToken(TokenType::Identifier);
        switch (theToken.value()[0]) {
            case E_PACKAGE:
                attributes.check(theToken.position(), package_->app());
                documentation.disallow();
                parsePackageImport(theToken.position());
                continue;
            case E_CROCODILE:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->app());
                parseProtocol(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_TURKEY:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->app());
                parseEnum(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_RADIO:
                attributes.check(theToken.position(), package_->app());
                documentation.disallow();
                package_->setRequiresBinary();
                if (package_->name() == "_") {
                    throw CompilerError(theToken.position(), "You may not set 📻 for the _ package.");
                }
                continue;
            case E_TRIANGLE_POINTED_DOWN: {
                attributes.check(theToken.position(), package_->app());
                TypeIdentifier alias = parseTypeIdentifier();
                Type type = package_->getRawType(parseTypeIdentifier(), false);
                package_->offerType(type, alias.name, alias.ns, false, theToken.position());
                continue;
            }
            case E_CRYSTAL_BALL:
                attributes.check(theToken.position(), package_->app());
                parseVersion(documentation, theToken.position());
                continue;
            case E_WALE:
                attributes.check(theToken.position(), package_->app());
                documentation.disallow();
                parseExtension(documentation, theToken.position());
                continue;
            case E_RABBIT:
                parseClass(documentation.get(), theToken, attributes.has(Attribute::Export),
                           attributes.has(Attribute::Final));
                continue;
            case E_DOVE_OF_PEACE:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->app());
                parseValueType(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_SCROLL:
                attributes.check(theToken.position(), package_->app());
                documentation.disallow();
                parseInclude(theToken.position());
                continue;
            case E_CHEQUERED_FLAG:
                attributes.check(theToken.position(), package_->app());
                parseStartFlag(documentation, theToken.position());
                break;
            default:
                throw CompilerError(theToken.position(), "Unexpected identifier ", utf8(theToken.value()));
        }
    }
}

TypeIdentifier DocumentParser::parseAndValidateNewTypeName() {
    auto parsedTypeName = parseTypeIdentifier();

    Type type = Type::nothingness();
    if (package_->lookupRawType(parsedTypeName, false, &type)) {
        auto str = type.toString(Type::nothingness());
        throw CompilerError(parsedTypeName.position, "Type ", str, " is already defined.");
    }

    return parsedTypeName;
}

void DocumentParser::parsePackageImport(const SourcePosition &p) {
    auto &nameToken = stream_.consumeToken(TokenType::Variable);
    auto &namespaceToken = stream_.consumeToken(TokenType::Identifier);

    package_->importPackage(utf8(nameToken.value()), namespaceToken.value(), p);
}

void DocumentParser::parseStartFlag(const Documentation &documentation, const SourcePosition &p) {
    if (package_->app()->hasStartFlagFunction()) {
        throw CompilerError(p, "Duplicate 🏁.");
    }

    auto function = new Function(std::u32string(1, E_CHEQUERED_FLAG), AccessLevel::Public, false,
                                 Type::nothingness(), package_, p, false,
                                 documentation.get(), false, false, FunctionType::Function);
    parseReturnType(function, Type::nothingness());
    if (function->returnType.type() != TypeType::Nothingness &&
        !function->returnType.compatibleTo(Type::integer(), Type::nothingness())) {
        throw CompilerError(p, "🏁 must either return ✨ or 🚂.");
    }
    stream_.requireIdentifier(E_GRAPES);
    try {
        auto ast = FunctionParser(package_, stream_, function->typeContext()).parse();
        function->setAst(ast);
    }
    catch (CompilerError &ce) {
        package_->app()->error(ce);
    }
    package_->registerFunction(function);

    package_->app()->setStartFlagFunction(function);
}

void DocumentParser::parseInclude(const SourcePosition &p) {
    auto &pathString = stream_.consumeToken(TokenType::String);
    auto relativePath = std::string(pathString.position().file);
    auto fileString = utf8(pathString.value());

    auto lastSlash = relativePath.find_last_of('/');
    if (lastSlash != relativePath.npos) {
        fileString = relativePath.substr(0, lastSlash) + "/" + fileString;
    }

    DocumentParser(package_, Lexer::lexFile(fileString)).parse();
}

void DocumentParser::parseVersion(const Documentation &documentation, const SourcePosition &p) {
    if (package_->validVersion()) {
        package_->app()->error(CompilerError(p, "Package version already declared."));
        return;
    }

    auto major = std::stoi(utf8(stream_.consumeToken(TokenType::Integer).value()));
    auto minor = std::stoi(utf8(stream_.consumeToken(TokenType::Integer).value()));

    package_->setPackageVersion(PackageVersion(major, minor));
    if (!package_->validVersion()) {
        throw CompilerError(p, "The provided package version is not valid.");
    }

    package_->setDocumentation(documentation.get());
}

void DocumentParser::parseExtension(const Documentation &documentation, const SourcePosition &p) {
    Type type = package_->getRawType(parseTypeIdentifier(), false);
    Type extendedType = type;

    auto extension = Extension(type, package_, p, documentation.get());
    if (type.typeDefinition()->package() != package_) {
        type = Type(&extension);
    }

    switch (extendedType.type()) {
        case TypeType::Class:
            ClassTypeBodyParser(type, package_, stream_, std::set<std::u32string>()).parse();
            break;
        case TypeType::ValueType:
            ValueTypeBodyParser(type, package_, stream_).parse();
            break;
        default:
            throw CompilerError(p, "Only classes and value types are extendable.");
    }
    package_->registerExtension(extension);
}

void DocumentParser::parseProtocol(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto protocol = new Protocol(parsedTypeName.name, package_, theToken.position(), documentation);

    parseGenericParameters(protocol, Type(protocol, false));

    stream_.requireIdentifier(E_GRAPES);

    auto protocolType = Type(protocol, false);
    package_->offerType(protocolType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        auto documentation = Documentation().parse(&stream_);
        auto &token = stream_.consumeToken(TokenType::Identifier);
        auto deprecated = stream_.consumeTokenIf(E_WARNING_SIGN);

        if (!token.isIdentifier(E_PIG)) {
            throw CompilerError(token.position(), "Only method declarations are allowed inside a protocol.");
        }

        auto &methodName = stream_.consumeToken(TokenType::Identifier, TokenType::Operator);

        auto method = new ProtocolFunction(methodName.value(), AccessLevel::Public, false, protocolType, package_,
                                           methodName.position(), false, documentation.get(), deprecated, false,
                                           FunctionType::ObjectMethod);
        parseParameters(method, protocolType, false);
        parseReturnType(method, protocolType);

        protocol->addMethod(method);
    }
    stream_.consumeToken(TokenType::Identifier);
}

void DocumentParser::parseEnum(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    Enum *enumeration = new Enum(parsedTypeName.name, package_, theToken.position(), documentation);

    auto type = Type(enumeration, false);
    package_->offerType(type, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    package_->registerValueType(enumeration);
    EnumTypeBodyParser(type, package_, stream_).parse();
}

void DocumentParser::parseClass(const std::u32string &documentation, const Token &theToken, bool exported, bool final) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto eclass = new Class(parsedTypeName.name, package_, theToken.position(), documentation, final);

    parseGenericParameters(eclass, Type(eclass, false));

    if (!stream_.nextTokenIs(E_GRAPES)) {
        auto classType = Type(eclass, false);  // New Type due to generic arguments now (partly) available.

        Type type = parseType(TypeContext(classType), TypeDynamism::GenericTypeVariables);
        if (type.type() != TypeType::Class && !type.optional() && !type.meta()) {
            throw CompilerError(parsedTypeName.position, "The superclass must be a class.");
        }
        if (type.eclass()->final()) {
            package_->app()->error(CompilerError(parsedTypeName.position, type.toString(classType),
                                                 " can’t be used as superclass as it was marked with 🔏."));
        }
        eclass->setSuperType(type);
    }

    auto classType = Type(eclass, false);  // New Type due to generic arguments now available.
    package_->offerType(classType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    package_->registerClass(eclass);

    auto requiredInits = eclass->superclass() != nullptr ? eclass->superclass()->requiredInitializers() : std::set<std::u32string>();
    ClassTypeBodyParser(classType, package_, stream_, requiredInits).parse();
}

void DocumentParser::parseValueType(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto valueType = new ValueType(parsedTypeName.name, package_, theToken.position(), documentation);

    if (stream_.consumeTokenIf(E_WHITE_CIRCLE)) {
        valueType->makePrimitive();
    }

    parseGenericParameters(valueType, Type(valueType, false));

    auto valueTypeContent = Type(valueType, false);
    package_->offerType(valueTypeContent, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    package_->registerValueType(valueType);
    ValueTypeBodyParser(valueTypeContent, package_, stream_).parse();
}

}  // namespace EmojicodeCompiler
