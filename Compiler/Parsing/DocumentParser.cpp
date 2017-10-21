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
#include "ProtocolTypeBodyParser.hpp"
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
                attributes.check(theToken.position(), package_->compiler());
                documentation.disallow();
                parsePackageImport(theToken.position());
                continue;
            case E_CROCODILE:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->compiler());
                parseProtocol(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_TURKEY:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->compiler());
                parseEnum(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_TRIANGLE_POINTED_DOWN: {
                attributes.check(theToken.position(), package_->compiler());
                TypeIdentifier alias = parseTypeIdentifier();
                Type type = package_->getRawType(parseTypeIdentifier(), false);
                package_->offerType(type, alias.name, alias.ns, false, theToken.position());
                continue;
            }
            case E_CRYSTAL_BALL:
                attributes.check(theToken.position(), package_->compiler());
                parseVersion(documentation, theToken.position());
                continue;
            case E_WALE:
                attributes.check(theToken.position(), package_->compiler());
                documentation.disallow();
                parseExtension(documentation, theToken.position());
                continue;
            case E_RABBIT:
                parseClass(documentation.get(), theToken, attributes.has(Attribute::Export),
                           attributes.has(Attribute::Final));
                continue;
            case E_DOVE_OF_PEACE:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->compiler());
                parseValueType(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case E_SCROLL:
                attributes.check(theToken.position(), package_->compiler());
                documentation.disallow();
                parseInclude(theToken.position());
                continue;
            case E_CHEQUERED_FLAG:
                attributes.check(theToken.position(), package_->compiler());
                parseStartFlag(documentation, theToken.position());
                break;
            default:
                throw CompilerError(theToken.position(), "Unexpected identifier ", utf8(theToken.value()));
        }
    }
}

TypeIdentifier DocumentParser::parseAndValidateNewTypeName() {
    auto parsedTypeName = parseTypeIdentifier();

    Type type = Type::noReturn();
    if (package_->lookupRawType(parsedTypeName, false, &type)) {
        auto str = type.toString(TypeContext());
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
    if (package_->hasStartFlagFunction()) {
        throw CompilerError(p, "Duplicate 🏁.");
    }

    auto function = package_->add(std::make_unique<Function>(std::u32string(1, E_CHEQUERED_FLAG), AccessLevel::Public,
                                                             false, Type::noReturn(), package_, p, false,
                                                             documentation.get(), false, false, FunctionType::Function));
    parseReturnType(function, TypeContext());
    if (function->returnType.type() != TypeType::NoReturn &&
        !function->returnType.compatibleTo(Type::integer(), TypeContext())) {
        throw CompilerError(p, "🏁 must either return ✨ or 🚂.");
    }
    stream_.consumeToken(TokenType::BlockBegin);
    try {
        auto ast = factorFunctionParser(package_, stream_, function->typeContext(), function)->parse();
        function->setAst(ast);
    }
    catch (CompilerError &ce) {
        package_->compiler()->error(ce);
    }
    package_->setStartFlagFunction(function);
}

void DocumentParser::parseInclude(const SourcePosition &p) {
    auto &pathString = stream_.consumeToken(TokenType::String);
    auto relativePath = std::string(pathString.position().file);
    auto originalPathString = utf8(pathString.value());
    auto fileString = originalPathString;

    auto lastSlash = relativePath.find_last_of('/');
    if (lastSlash != relativePath.npos) {
        fileString = relativePath.substr(0, lastSlash) + "/" + fileString;
    }

    package_->includeDocument(fileString, originalPathString);
}

void DocumentParser::parseVersion(const Documentation &documentation, const SourcePosition &p) {
    if (package_->validVersion()) {
        package_->compiler()->error(CompilerError(p, "Package version already declared."));
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

    auto extension = package_->add(std::make_unique<Extension>(type, package_, p, documentation.get()));
    Type extendedType = Type(extension);

    switch (type.type()) {
        case TypeType::Class:
            ClassTypeBodyParser(extendedType, package_, stream_, interface_, std::set<std::u32string>()).parse();
            break;
        case TypeType::ValueType:
            ValueTypeBodyParser(extendedType, package_, stream_, interface_).parse();
            break;
        default:
            throw CompilerError(p, "Only classes and value types are extendable.");
    }
}

void DocumentParser::parseProtocol(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto protocol = package_->add(std::make_unique<Protocol>(parsedTypeName.name, package_,
                                                             theToken.position(), documentation, exported));

    parseGenericParameters(protocol, TypeContext(Type(protocol, false)));

    auto protocolType = Type(protocol, false);
    package_->offerType(protocolType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    ProtocolTypeBodyParser(protocolType, package_, stream_, interface_).parse();
}

void DocumentParser::parseEnum(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto enumUniq = std::make_unique<Enum>(parsedTypeName.name, package_, theToken.position(), documentation, exported);
    Enum *enumeration = enumUniq.get();
    package_->add(std::move(enumUniq));

    auto type = Type(enumeration, false);
    package_->offerType(type, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    EnumTypeBodyParser(type, package_, stream_, interface_).parse();
}

void DocumentParser::parseClass(const std::u32string &documentation, const Token &theToken, bool exported, bool final) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto eclass = package_->add(std::make_unique<Class>(parsedTypeName.name, package_, theToken.position(),
                                                        documentation, exported, final));

    parseGenericParameters(eclass, TypeContext(Type(eclass, false)));

    if (!stream_.nextTokenIs(TokenType::BlockBegin)) {
        auto classType = Type(eclass, false);  // New Type due to generic arguments now (partly) available.

        Type type = parseType(TypeContext(classType), TypeDynamism::GenericTypeVariables);
        if (type.type() != TypeType::Class && !type.optional() && !type.meta()) {
            throw CompilerError(parsedTypeName.position, "The superclass must be a class.");
        }
        if (type.eclass()->final()) {
            package_->compiler()->error(CompilerError(parsedTypeName.position, type.toString(TypeContext(classType)),
                                                 " can’t be used as superclass as it was marked with 🔏."));
        }
        eclass->setSuperType(type);
    }

    auto classType = Type(eclass, false);  // New Type due to generic arguments now available.
    package_->offerType(classType, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());

    auto requiredInits = eclass->superclass() != nullptr ? eclass->superclass()->requiredInitializers() : std::set<std::u32string>();
    ClassTypeBodyParser(classType, package_, stream_, interface_, requiredInits).parse();
}

void DocumentParser::parseValueType(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto valueType = package_->add(std::make_unique<ValueType>(parsedTypeName.name, package_,
                                                               theToken.position(), documentation, exported));

    if (stream_.consumeTokenIf(E_WHITE_CIRCLE)) {
        valueType->makePrimitive();
    }

    parseGenericParameters(valueType, TypeContext(Type(valueType, false)));

    auto valueTypeContent = Type(valueType, false);
    package_->offerType(valueTypeContent, parsedTypeName.name, parsedTypeName.ns, exported, theToken.position());
    ValueTypeBodyParser(valueTypeContent, package_, stream_, interface_).parse();
}

}  // namespace EmojicodeCompiler
