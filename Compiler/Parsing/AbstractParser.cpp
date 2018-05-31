//
//  AbstractParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "AbstractParser.hpp"
#include "CompatibilityInfoProvider.hpp"
#include "Compiler.hpp"
#include "Emojis.h"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Parsing/CompatibleFunctionParser.hpp"
#include "Types/Protocol.hpp"
#include <map>
#include <vector>

namespace EmojicodeCompiler {

TypeIdentifier AbstractParser::parseTypeIdentifier() {
    std::u32string namespase = stream_.consumeTokenIf(E_ORANGE_TRIANGLE) ? parseTypeEmoji().value() : kDefaultNamespace;
    auto typeName = parseTypeEmoji();
    return TypeIdentifier(typeName.value(), namespase, typeName.position());
}

Token AbstractParser::parseTypeEmoji() const {
    if (stream_.nextTokenIs(E_CANDY) || stream_.nextTokenIs(E_MEDIUM_BLACK_CIRCLE) ||
        stream_.nextTokenIs(E_MEDIUM_WHITE_CIRCLE) || stream_.nextTokenIs(E_LARGE_BLUE_CIRCLE) ||
        stream_.nextTokenIs(E_BENTO_BOX) || stream_.nextTokenIs(E_ORANGE_TRIANGLE)) {
        auto token = stream_.consumeToken();
        throw CompilerError(token.position(), "Unexpected identifier ", utf8(token.value()), " with special meaning.");
    }
    if (stream_.nextTokenIs(TokenType::ForIn)) {
        return stream_.consumeToken();
    }
    return stream_.consumeToken(TokenType::Identifier);
}

std::unique_ptr<FunctionParser> AbstractParser::factorFunctionParser(Package *pkg, TokenStream &stream,
                                                                     TypeContext context, Function *function) {
    if (package_->compatibilityMode()) {
        package_->compatibilityInfoProvider()->selectFunction(function);
        return std::make_unique<CompatibleFunctionParser>(pkg, stream, context);
    }
    return std::make_unique<FunctionParser>(pkg, stream, context);
}

Type AbstractParser::parseType(const TypeContext &typeContext) {
    if (stream_.nextTokenIs(TokenType::Class) || stream_.nextTokenIs(TokenType::Enumeration) ||
            stream_.nextTokenIs(TokenType::ValueType) || stream_.nextTokenIs(TokenType::Protocol)) {
        return parseTypeAsValueType(typeContext);
    }

    bool optional = stream_.consumeTokenIf(E_CANDY);

    if (stream_.nextTokenIs(E_MEDIUM_WHITE_CIRCLE)) {
        auto token = stream_.consumeToken();
        if (optional) {
            throw CompilerError(token.position(), "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
        }
        return Type::something().applyMinimalBoxing();
    }
    if (stream_.nextTokenIs(TokenType::Error)) {
        return parseErrorType(optional, typeContext);
    }
    return parseTypeMain(typeContext).optionalized(optional).applyMinimalBoxing();
}

Type AbstractParser::parseTypeAsValueType(const TypeContext &typeContext) {
    auto token = stream_.consumeToken();
    auto type = parseType(typeContext);
    validateTypeAsValueType(token, type, typeContext);
    return Type(MakeTypeAsValue, type);
}

void AbstractParser::validateTypeAsValueType(const Token &token, const Type &type, const TypeContext &typeContext) {
    if (token.type() == TokenType::Class && type.type() != TypeType::Class) {
        throw CompilerError(token.position(), "Expected a class type but got ", type.toString(typeContext), ".");
    }
    if (token.type() == TokenType::Protocol && type.type() != TypeType::Protocol) {
        throw CompilerError(token.position(), "Expected a protocol type but got ", type.toString(typeContext), ".");
    }
    if (token.type() == TokenType::ValueType && type.type() != TypeType::ValueType) {
        throw CompilerError(token.position(), "Expected a value type but got ", type.toString(typeContext), ".");
    }
    if (token.type() == TokenType::Enumeration && type.type() != TypeType::Enum) {
        throw CompilerError(token.position(), "Expected a class type but got ", type.toString(typeContext), ".");
    }
}

Type AbstractParser::parseTypeMain(const TypeContext &typeContext) {
    if (stream_.consumeTokenIf(TokenType::BlockBegin)) {
        return parseCallableType(typeContext);
    }
    if (stream_.nextTokenIs(TokenType::Variable)) {
        return parseGenericVariable(typeContext);
    }
    if (stream_.nextTokenIs(E_BENTO_BOX)) {
        return parseMultiProtocol(typeContext);
    }
    if (stream_.consumeTokenIf(E_LARGE_BLUE_CIRCLE)) {
        return Type::someobject();
    }

    auto parsedTypeIdentifier = parseTypeIdentifier();
    auto type = package_->getRawType(parsedTypeIdentifier);
    if (type.canHaveGenericArguments()) {
        parseGenericArgumentsForType(&type, typeContext, parsedTypeIdentifier.position);
    }
    return type;
}

Type AbstractParser::parseMultiProtocol(const TypeContext &typeContext) {
    auto bentoToken = stream_.consumeToken(TokenType::Identifier);

    std::vector<Type> protocols;
    while (stream_.nextTokenIsEverythingBut(E_BENTO_BOX)) {
        auto protocolType = parseType(typeContext).unboxed();
        if (protocolType.type() != TypeType::Protocol) {
            package_->compiler()->error(CompilerError(bentoToken.position(),
                                                      "🍱 may only consist of non-optional protocol types."));
            continue;
        }
        protocols.push_back(protocolType.unboxed());
    }
    stream_.consumeToken(TokenType::Identifier);

    if (protocols.empty()) {
        throw CompilerError(bentoToken.position(), "An empty 🍱 is invalid.");
    }
    return Type(std::move(protocols));
}

Type AbstractParser::parseCallableType(const TypeContext &typeContext) {
    std::vector<Type> params;
    while (stream_.nextTokenIsEverythingBut(TokenType::BlockEnd) &&
            stream_.nextTokenIsEverythingBut(TokenType::RightProductionOperator)) {
        params.emplace_back(parseType(typeContext));
    }
    auto returnType = stream_.consumeTokenIf(TokenType::RightProductionOperator) ? parseType(typeContext)
                                                                                 : Type::noReturn();
    stream_.consumeToken(TokenType::BlockEnd);
    return Type(returnType, params);
}

Type AbstractParser::parseGenericVariable(const TypeContext &typeContext) {
    auto varToken = stream_.consumeToken(TokenType::Variable);

    Type type = Type::noReturn();
    if (typeContext.function() != nullptr && typeContext.function()->fetchVariable(varToken.value(), &type)) {
        return type;
    }

    auto callee = typeContext.calleeType().unboxed();
    if (callee.canHaveGenericArguments() && callee.typeDefinition()->fetchVariable(varToken.value(), &type)) {
        return type;
    }

    throw CompilerError(varToken.position(), "No such generic type variable \"", utf8(varToken.value()), "\".");
}

Type AbstractParser::parseErrorEnumType(const TypeContext &typeContext, const SourcePosition &p) {
    auto errorType = parseType(typeContext);
    if (errorType.type() != TypeType::Enum) {
        throw CompilerError(p, "Error type must be a non-optional 🦃.");
    }
    return errorType;
}

Type AbstractParser::parseErrorType(bool optional, const TypeContext &typeContext) {
    auto token = stream_.consumeToken(TokenType::Error);
    Type errorEnum = parseErrorEnumType(typeContext, token.position());
    if (optional) {
        throw CompilerError(token.position(), "The error type itself cannot be an optional. "
                            "Maybe you meant to make the contained type an optional?");
    }
    return parseType(typeContext).errored(errorEnum);
}

void AbstractParser::parseGenericArgumentsForType(Type *type, const TypeContext &typeContext, const SourcePosition &p) {
    auto typeDef = type->typeDefinition();
    auto offset = typeDef->superGenericArguments().size();
    auto args = typeDef->superGenericArguments();

    size_t count = 0;
    for (; stream_.nextTokenIs(E_SPIRAL_SHELL) && count < typeDef->genericParameters().size(); count++) {
        auto token = stream_.consumeToken(TokenType::Identifier);

        Type argument = parseType(typeContext);
        if (!argument.compatibleTo(typeDef->constraintForIndex(offset + count), typeContext)) {
            throw CompilerError(token.position(), "Generic argument for ", argument.toString(typeContext),
                                " is not compatible to constraint ",
                                typeDef->constraintForIndex(offset + count).toString(typeContext), ".");
        }
        args.emplace_back(argument);
    }

    if (count != typeDef->genericParameters().size()) {
        throw CompilerError(p, "Type ", type->toString(TypeContext()), " requires ",
                            typeDef->genericParameters().size(), " generic arguments, but ", count, " were given.");
    }

    type->setGenericArguments(std::move(args));
}

void AbstractParser::parseParameters(Function *function, const TypeContext &typeContext, bool initializer) {
    bool argumentToVariable;
    std::vector<Parameter> params;

    while ((argumentToVariable = stream_.nextTokenIs(E_BABY_BOTTLE)) || stream_.nextTokenIs(TokenType::Variable)) {
        if (argumentToVariable) {
            auto token = stream_.consumeToken(TokenType::Identifier);
            if (!initializer) {
                throw CompilerError(token.position(), "🍼 can only be used with initializers.");
            }
        }

        auto variableToken = stream_.consumeToken(TokenType::Variable);
        auto type = parseType(typeContext);

        params.emplace_back(variableToken.value(), type);

        if (argumentToVariable) {
            dynamic_cast<Initializer *>(function)->addArgumentToVariable(variableToken.value(), variableToken.position());
        }
    }
    function->setParameters(std::move(params));
}

void AbstractParser::parseReturnType(Function *function, const TypeContext &typeContext) {
    if (stream_.consumeTokenIf(TokenType::RightProductionOperator)) {
        function->setReturnType(parseType(typeContext));
    }
}

}  // namespace EmojicodeCompiler
