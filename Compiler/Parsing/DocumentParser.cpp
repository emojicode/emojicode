//
//  DocumentParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 24/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "DocumentParser.hpp"
#include "FunctionParser.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeContext.hpp"
#include "Types/ValueType.hpp"
#include <cstring>

namespace EmojicodeCompiler {

void DocumentParser::parse() {
    while (stream_.hasMoreTokens()) {
        auto documentation = Documentation().parse(&stream_);
        auto attributes = PackageAttributeParser().parse(&stream_);
        auto theToken = stream_.consumeToken();

        switch (theToken.type()) {
            case TokenType::Class:
                parseClass(documentation.get(), theToken, attributes.has(Attribute::Export),
                           attributes.has(Attribute::Final), attributes.has(Attribute::Foreign));
                continue;
            case TokenType::Protocol:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->compiler());
                parseProtocol(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case TokenType::Enumeration:
                attributes.allow(Attribute::Export).check(theToken.position(), package_->compiler());
                parseEnum(documentation.get(), theToken, attributes.has(Attribute::Export));
                continue;
            case TokenType::ValueType:
                attributes.allow(Attribute::Export).allow(Attribute::Foreign)
                    .check(theToken.position(), package_->compiler());
                parseValueType(documentation.get(), theToken, attributes.has(Attribute::Export),
                               attributes.has(Attribute::Foreign));
                continue;
            case TokenType::PackageDocumentationComment:
                attributes.check(theToken.position(), package_->compiler());
                documentation.disallow();
                package_->setDocumentation(theToken.value());
                continue;
            case TokenType::Identifier:
                switch (theToken.value()[0]) {
                    case E_PACKAGE:
                        attributes.check(theToken.position(), package_->compiler());
                        documentation.disallow();
                        parsePackageImport(theToken.position());
                        continue;
                    case E_WALE:
                        attributes.check(theToken.position(), package_->compiler());
                        documentation.disallow();
                        parseExtension(documentation, theToken.position());
                        continue;
                    case E_SCROLL:
                        attributes.check(theToken.position(), package_->compiler());
                        documentation.disallow();
                        parseInclude(theToken.position());
                        continue;
                    case E_CHEQUERED_FLAG:
                        attributes.check(theToken.position(), package_->compiler());
                        parseStartFlag(documentation, theToken.position());
                        continue;
                    case E_LINK_SYMBOL:
                        attributes.check(theToken.position(), package_->compiler());
                        parseLinkHints(theToken.position());
                        continue;
                    default:
                        break;  // and fallthrough to the error
                }
            default:
                throw CompilerError(theToken.position(), "Unexpected token ", theToken.stringName());
        }
    }
}

TypeIdentifier DocumentParser::parseAndValidateNewTypeName() {
    auto parsedTypeName = parseTypeIdentifier();

    Type type = Type::noReturn();
    if (package_->lookupRawType(parsedTypeName, &type)) {
        auto str = type.toString(TypeContext());
        throw CompilerError(parsedTypeName.position, "Type ", str, " is already defined.");
    }

    return parsedTypeName;
}

void DocumentParser::parsePackageImport(const SourcePosition &p) {
    auto nameToken = stream_.consumeToken(TokenType::Variable);
    auto namespaceToken = stream_.consumeToken(TokenType::Identifier);

    package_->importPackage(utf8(nameToken.value()), namespaceToken.value(), p);
}

void DocumentParser::parseStartFlag(const Documentation &documentation, const SourcePosition &p) {
    if (package_->hasStartFlagFunction()) {
        package_->compiler()->error(CompilerError(p, "Duplicate 🏁."));
        return;
    }

    auto function = package_->add(std::make_unique<Function>(std::u32string(1, E_CHEQUERED_FLAG), AccessLevel::Public,
                                                             false, nullptr, package_, p, false,
                                                             documentation.get(), false, false, Mood::Imperative, false,
                                                             FunctionType::Function));
    parseReturnType(function);
    stream_.consumeToken(TokenType::BlockBegin);

    function->setAst(FunctionParser(package_, stream_).parse());
    package_->setStartFlagFunction(function);
}

void DocumentParser::parseInclude(const SourcePosition &p) {
    auto pathString = stream_.consumeToken(TokenType::String);
    auto relativePath = std::string(pathString.position().file->path());
    auto originalPathString = utf8(pathString.value());
    auto fileString = originalPathString;

    auto lastSlash = relativePath.find_last_of('/');
    if (lastSlash != relativePath.npos) {
        fileString = relativePath.substr(0, lastSlash) + "/" + fileString;
    }

    package_->includeDocument(fileString, originalPathString);
}

void DocumentParser::parseLinkHints(const SourcePosition &p) {
    if (!package_->linkHints().empty()) {
        package_->compiler()->error(CompilerError(p, "Link hints were already provided."));
    }
    std::vector<std::string> hints;
    do {
        hints.emplace_back(utf8(stream_.consumeToken(TokenType::String).value()));
    } while (stream_.nextTokenIsEverythingBut(E_LINK_SYMBOL));
    stream_.consumeToken();
    package_->setLinkHints(std::move(hints));
}

void DocumentParser::parseExtension(const Documentation &documentation, const SourcePosition &p) {
    Type type = package_->getRawType(parseTypeIdentifier());

    auto extension = package_->add(std::make_unique<Extension>(type, package_, p, documentation.get()));

    switch (type.type()) {
        case TypeType::Class:
            TypeBodyParser<Class>(extension->extendedType().klass(), package_, stream_, interface_).parse();
            break;
        case TypeType::ValueType:
            TypeBodyParser<ValueType>(extension->extendedType().valueType(), package_, stream_, interface_).parse();
            break;
        default:
            throw CompilerError(p, "Only classes and value types are extendable.");
    }
}

void DocumentParser::parseProtocol(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto protocol = package_->add(std::make_unique<Protocol>(parsedTypeName.name, package_,
                                                             theToken.position(), documentation, exported));

    parseGenericParameters(protocol);

    offerAndParseBody(protocol, parsedTypeName, theToken.position());
}

void DocumentParser::parseEnum(const std::u32string &documentation, const Token &theToken, bool exported) {
    auto parsedTypeName = parseAndValidateNewTypeName();
    auto enumUniq = std::make_unique<Enum>(parsedTypeName.name, package_, theToken.position(), documentation, exported);
    Enum *enumeration = enumUniq.get();
    package_->add(std::move(enumUniq));

    offerAndParseBody(enumeration, parsedTypeName, theToken.position());
}

void DocumentParser::parseClass(const std::u32string &documentation, const Token &theToken, bool exported, bool final,
                                bool foreign) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto eclass = package_->add(std::make_unique<Class>(parsedTypeName.name, package_, theToken.position(),
                                                        documentation, exported, final, foreign));

    parseGenericParameters(eclass);

    if (!stream_.nextTokenIs(TokenType::BlockBegin)) {
        eclass->setSuperType(parseType());
    }

    offerAndParseBody(eclass, parsedTypeName, theToken.position());
}

void DocumentParser::parseValueType(const std::u32string &documentation, const Token &theToken, bool exported,
                                    bool primitive) {
    auto parsedTypeName = parseAndValidateNewTypeName();

    auto valueType = package_->add(std::make_unique<ValueType>(parsedTypeName.name, package_, theToken.position(),
                                                               documentation, exported, primitive));
    parseGenericParameters(valueType);
    offerAndParseBody(valueType, parsedTypeName, theToken.position());
}

}  // namespace EmojicodeCompiler
