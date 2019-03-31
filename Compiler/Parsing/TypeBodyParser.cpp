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
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include <type_traits>

namespace EmojicodeCompiler {

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseFunctionBody(Function *function) {
    if (stream_.consumeTokenIf(E_RADIO)) {
        auto token = stream_.consumeToken(TokenType::String);
        if (token.value().empty()) {
            throw CompilerError(token.position(), "The external name must not be empty.");
        }
        function->setExternalName(utf8(token.value()));
        return;
    }

    if (interface_) {
        if (!stream_.nextTokenIs(TokenType::BlockBegin)) {
            function->makeExternal();
            return;
        }
        if (!function->isInline()) {
            throw CompilerError(stream_.consumeToken().position(), "Interface can only define inline functions.");
        }
    }

    stream_.consumeToken(TokenType::BlockBegin);
    function->setAst(FunctionParser(package_, stream_).parse());
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseFunction(Function *function, bool inititalizer, bool escaping) {
    parseGenericParameters(function);
    parseParameters(function, inititalizer);
    if (!inititalizer) {
        parseReturnType(function);
    }
    bool errorProne = parseErrorType(function);
    if (errorProne && inititalizer && !std::is_same<TypeDef, Class>::value) {
        throw CompilerError(function->position(), "Only classes can have error-prone initializers.");
    }
    parseFunctionBody(function);
    if (escaping) {
        function->setMemoryFlowTypeForThis(MFFlowCategory::Escaping);
    }
    else if (function->isExternal()) {
        function->setMemoryFlowTypeForThis(MFFlowCategory::Borrowing);
    }
}

template <typename TypeDef>
AccessLevel TypeBodyParser<TypeDef>::readAccessLevel() {
    if (stream_.consumeTokenIf(E_CLOSED_LOCK_WITH_KEY)) {
        return AccessLevel::Protected;
    }
    if (stream_.consumeTokenIf(E_LOCK)) {
        return AccessLevel::Private;
    }
    if (stream_.consumeTokenIf(E_OPEN_LOCK)) {
        return AccessLevel::Public;
    }
    return AccessLevel::Default;
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseProtocolConformance(const SourcePosition &p) {
    typeDef_->addProtocol(parseType());
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    throw CompilerError(p, "Enum values can only be declared in enums.");
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseInstanceVariable(const SourcePosition &p) {
    auto variableName = stream_.consumeToken(TokenType::Variable);
    auto instanceVar = InstanceVariableDeclaration(variableName.value(), parseType(), variableName.position());
    if (stream_.consumeTokenIf(TokenType::LeftProductionOperator)) {
        instanceVar.expr = FunctionParser(package_, stream_).parseExpr(0);
    }
    typeDef_->addInstanceVariable(instanceVar);
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::doParseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                 const Documentation &documentation, AccessLevel access, Mood mood,
                                 const SourcePosition &p) {
    attributes.allow(Attribute::Deprecated).allow(Attribute::StaticOnType).allow(Attribute::Unsafe)
            .allow(Attribute::Escaping).allow(Attribute::Inline).check(p, package_->compiler());

    if (attributes.has(Attribute::StaticOnType)) {
        auto typeMethod = std::make_unique<Function>(name, access, attributes.has(Attribute::Final), typeDef_,
                                                     package_, p, attributes.has(Attribute::Override),
                                                     documentation.get(), attributes.has(Attribute::Deprecated),
                                                     true, mood, attributes.has(Attribute::Unsafe),
                                                     std::is_same<TypeDef, Class>::value ?
                                                     FunctionType::ClassMethod : FunctionType::Function,
                                                     attributes.has(Attribute::Inline));
        parseFunction(typeMethod.get(), false, attributes.has(Attribute::Escaping));
        typeDef_->addTypeMethod(std::move(typeMethod));
    }
    else {
        auto mutating = !std::is_same<TypeDef, ValueType>::value || attributes.has(Attribute::Mutating);
        auto method = std::make_unique<Function>(name, access, attributes.has(Attribute::Final), typeDef_,
                                                 package_, p, attributes.has(Attribute::Override), documentation.get(),
                                                 attributes.has(Attribute::Deprecated), mutating, mood,
                                                 attributes.has(Attribute::Unsafe),
                                                 std::is_same<TypeDef, Class>::value ? FunctionType::ObjectMethod :
                                                 FunctionType::ValueTypeMethod, attributes.has(Attribute::Inline));
        parseFunction(method.get(), false, attributes.has(Attribute::Escaping));
        typeDef_->addMethod(std::move(method));
    }
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                          const Documentation &documentation, AccessLevel access, Mood mood,
                                          const SourcePosition &p) {
    doParseMethod(name, std::move(attributes), documentation, access, mood, p);
}

template <typename TypeDef>
Initializer* TypeBodyParser<TypeDef>::doParseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                              const Documentation &documentation, AccessLevel access,
                                              const SourcePosition &p) {
    attributes.allow(Attribute::Unsafe).allow(Attribute::Escaping).allow(Attribute::Inline)
        .check(p, package_->compiler());

    auto initializer = std::make_unique<Initializer>(name, access, attributes.has(Attribute::Final), typeDef_,
                                                     package_, p, attributes.has(Attribute::Override),
                                                     documentation.get(), attributes.has(Attribute::Deprecated),
                                                     attributes.has(Attribute::Required),
                                                     attributes.has(Attribute::Unsafe),
                                                     std::is_same<TypeDef, Class>::value ?
                                                     FunctionType::ObjectInitializer :
                                                     FunctionType::ValueTypeInitializer,
                                                     attributes.has(Attribute::Inline));
    parseFunction(initializer.get(), true, attributes.has(Attribute::Escaping));
    return typeDef_->addInitializer(std::move(initializer));
}

template <typename TypeDef>
Initializer* TypeBodyParser<TypeDef>::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                       const Documentation &documentation, AccessLevel access,
                                                       const SourcePosition &p) {
    return doParseInitializer(name, std::move(attributes), documentation, access, p);
}

void checkOperatorDefinition(Compiler *compiler, const Token &token) {
    if (!canOperatorBeDefined(token.value())) {
        compiler->error(CompilerError(token.position(), "Operator ", utf8(token.value()), " cannot be defined."));
    }
}

template <typename TypeDef>
void TypeBodyParser<TypeDef>::parse() {
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
                            token.type() == TokenType::EndArgumentList ? Mood::Imperative : Mood::Interogative,
                            token.position());
                break;
            }
            case TokenType::Operator:
                checkOperatorDefinition(package_->compiler(), token);
                parseMethod(token.value(), attributes, documentation, accessLevel, Mood::Imperative, token.position());
                break;
            case TokenType::RightProductionOperator:
                parseMethod(stream_.consumeToken(TokenType::Identifier).value(), attributes, documentation, accessLevel,
                            Mood::Assignment, token.position());
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
                    && !stream_.nextTokenIs(E_BABY_BOTTLE) && !stream_.nextTokenIs(E_OPEN_LOCK)
                    && !stream_.nextTokenIs(E_CLOSED_LOCK_WITH_KEY)
                    && !stream_.nextTokenIs(E_LOCK) && !stream_.nextTokenIs(E_CONSTRUCTION_SIGN)) {
                    name = stream_.consumeToken(TokenType::Identifier).value();
                }
                parseInitializer(name, attributes, documentation, accessLevel, token.position());
                break;
            }
            case TokenType::Protocol:
                attributes.check(token.position(), package_->compiler());
                documentation.disallow();
                parseProtocolConformance(token.position());
                break;
            case TokenType::Identifier:
                switch (token.value().front()) {
                    case E_RADIO_BUTTON:
                        attributes.check(token.position(), package_->compiler());
                        parseEnumValue(token.position(), documentation);
                        continue;
                    case E_RECYCLING_SYMBOL:
                        attributes.check(token.position(), package_->compiler());
                        parseDeinitializer(token.position());
                        continue;
                    default:
                        break;  // and fallthrough to the error
                }
            default:
                throw CompilerError(token.position(), "Unexpected token ", token.stringName());
        }
    }
    stream_.consumeToken(TokenType::BlockEnd);
}

template<>
void TypeBodyParser<Enum>::parseEnumValue(const SourcePosition &p, const Documentation &documentation) {
    auto token = stream_.consumeToken(TokenType::Identifier);
    typeDef_->addValueFor(token.value(), token.position(), documentation.get());
}

template<>
void TypeBodyParser<Enum>::parseInstanceVariable(const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have instance variable.");
}

template<>
Initializer* TypeBodyParser<Enum>::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                    const Documentation &documentation, AccessLevel access,
                                                    const SourcePosition &p) {
    throw CompilerError(p, "Enums cannot have custom initializers.");
}

template<>
void TypeBodyParser<Class>::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                        const Documentation &documentation, AccessLevel access, Mood mood,
                                        const SourcePosition &p) {
    doParseMethod(name, attributes.allow(Attribute::Final).allow(Attribute::Override), documentation,
                  access, mood, p);
}

template<>
void TypeBodyParser<ValueType>::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                            const Documentation &documentation, AccessLevel access, Mood mood,
                                            const SourcePosition &p) {
    if (!attributes.has(Attribute::StaticOnType)) {
        attributes.allow(Attribute::Mutating);
    }
    doParseMethod(name, attributes, documentation, access, mood, p);
}

template<>
Initializer* TypeBodyParser<Class>::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                     const Documentation &documentation, AccessLevel access,
                                                     const SourcePosition &p) {
    return doParseInitializer(name, attributes.allow(Attribute::Required), documentation, access, p);
}

template<>
void TypeBodyParser<Protocol>::parseMethod(const std::u32string &name, TypeBodyAttributeParser attributes,
                                           const Documentation &documentation, AccessLevel access, Mood mood,
                                           const SourcePosition &p) {
    auto method = std::make_unique<Function>(name, AccessLevel::Public, false, typeDef_, package_,
                                             p, false, documentation.get(),
                                             attributes.has(Attribute::Deprecated), false, mood, false,
                                             FunctionType::ObjectMethod, false);
    parseParameters(method.get(), false);
    parseReturnType(method.get());

    method->setMemoryFlowTypeForThis(MFFlowCategory::Escaping);
    for (size_t i = 0; i < method->parameters().size(); i++) {
        method->setParameterMFType(i, MFFlowCategory::Escaping);
    }

    typeDef_->addMethod(std::move(method));
}

template<>
Initializer* TypeBodyParser<Protocol>::parseInitializer(const std::u32string &name, TypeBodyAttributeParser attributes,
                                                        const Documentation &documentation, AccessLevel access,
                                                        const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

template<>
void TypeBodyParser<Protocol>::parseInstanceVariable(const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

template<>
void TypeBodyParser<Protocol>::parseProtocolConformance(const SourcePosition &p) {
    throw CompilerError(p, "Only method declarations are allowed inside a protocol.");
}

template<typename TypeDef>
void TypeBodyParser<TypeDef>::parseDeinitializer(const SourcePosition &p) {
    throw CompilerError(p, "Only classes can specify a deinitializer.");
}

template<>
void TypeBodyParser<Class>::parseDeinitializer(const SourcePosition &p) {
    if (typeDef_->deinitializer()->ast() != nullptr) {
        package_->compiler()->error(CompilerError(p, "Deinitializer was already specified."));
    }
    auto parser = FunctionParser(package_, stream_);
    stream_.consumeToken(TokenType::BlockBegin);
    typeDef_->deinitializer()->setAst(parser.parse());
}

template class TypeBodyParser<Enum>;
template class TypeBodyParser<ValueType>;
template class TypeBodyParser<Protocol>;
template class TypeBodyParser<Class>;

}  // namespace EmojicodeCompiler
