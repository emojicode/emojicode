//
// Created by Theo Weidmann on 04.06.18.
//

#include "ASTType.hpp"
#include "Compiler.hpp"
#include "Functions/Function.hpp"
#include "Lex/Token.hpp"
#include "Package/Package.hpp"
#include "Parsing/AbstractParser.hpp"
#include "Types/Type.hpp"
#include "Types/TypeContext.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

Type& ASTType::analyseType(const TypeContext &typeContext, bool allowReference) {
    if (!wasAnalysed()) {
        type_ = getType(typeContext).applyMinimalBoxing().optionalized(optional_);
        if (reference_) {
            if (!allowReference) {
                package()->compiler()->error(CompilerError(position(), "Reference not allowed here."));
            }
            if (!type_.isReferenceUseful() && type_.type() != TypeType::GenericVariable &&
                type_.type() != TypeType::LocalGenericVariable) {
                package()->compiler()->warn(position(), "Reference is not useful.");
            }
            type_.setReference();
        }
        if (type_.type() == TypeType::Optional && type_.isReference()) {
            package()->compiler()->error(CompilerError(position(), "Optional references are not supported."));
        }
        package_ = nullptr;
    }
    return type_;
}

Type ASTTypeId::getType(const TypeContext &typeContext) const {
    auto type = package()->getRawType(TypeIdentifier(name_, namespace_, position()));

    auto typeDef = type.typeDefinition();

    auto args = typeDef->superGenericArguments();
    for (auto &arg : genericArgs_) {
        args.emplace_back(arg->analyseType(typeContext));
    }
    typeDef->requestReificationAndCheck(typeContext, args, position());
    type.setGenericArguments(std::move(args));
    return type;
}

Type ASTMultiProtocol::getType(const TypeContext &typeContext) const {
    std::vector<Type> protocols;
    protocols.reserve(protocols_.size());

    for (auto &protocol : protocols_) {
        auto protocolType = protocol->analyseType(typeContext).unboxed();
        if (protocolType.type() != TypeType::Protocol) {
            package()->compiler()->error(CompilerError(position(),
                                                       "🍱 may only consist of non-optional protocol types."));
            continue;
        }
        protocols.push_back(protocolType.unboxed());
    }

    if (protocols.empty()) {
        throw CompilerError(position(), "An empty 🍱 is invalid.");
    }

    return Type(std::move(protocols));
}

Type ASTTypeValueType::getType(const TypeContext &typeContext) const {
    auto type = type_->analyseType(typeContext);
    checkTypeValue(tokenType_, type, typeContext, position());
    return Type(MakeTypeAsValue, type);
}

void ASTTypeValueType::checkTypeValue(TokenType tokenType, const Type &type, const TypeContext &typeContext,
                                      const SourcePosition &p) {
    if (tokenType == TokenType::Class && type.type() != TypeType::Class) {
        throw CompilerError(p, "Expected a class type but got ", type.toString(typeContext), ".");
    }
    if (tokenType == TokenType::Protocol && type.type() != TypeType::Protocol) {
        throw CompilerError(p, "Expected a protocol type but got ", type.toString(typeContext), ".");
    }
    if (tokenType == TokenType::ValueType && type.type() != TypeType::ValueType) {
        throw CompilerError(p, "Expected a value type but got ", type.toString(typeContext), ".");
    }
    if (tokenType == TokenType::Enumeration && type.type() != TypeType::Enum) {
        throw CompilerError(p, "Expected a class type but got ", type.toString(typeContext), ".");
    }
}

std::string ASTTypeValueType::toString(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::Class:
            return "🐇";
        case TokenType::ValueType:
            return "🕊";
        case TokenType::Enumeration:
            return "🦃";
        case TokenType::Protocol:
            return "🐊";
        default:
            throw std::logic_error("TokenType cannot produce Type Value");
    }
}

Type ASTGenericVariable::getType(const TypeContext &typeContext) const {
    Type type = Type::noReturn();
    if (typeContext.function() != nullptr && typeContext.function()->fetchVariable(name_, &type)) {
        return type;
    }

    auto callee = typeContext.calleeType().unboxed();
    if (callee.type() == TypeType::TypeAsValue) {
        callee = callee.typeOfTypeValue();
    }
    if (callee.canHaveGenericArguments() && callee.typeDefinition()->fetchVariable(name_, &type)) {
        return type;
    }

    throw CompilerError(position(), "No such generic type variable \"", utf8(name_), "\".");
}

Type ASTCallableType::getType(const TypeContext &typeContext) const {
    auto returnType = return_ == nullptr ? Type::noReturn() : return_->analyseType(typeContext);
    auto errorType = errorType_ == nullptr ? Type::noReturn() : errorType_->analyseType(typeContext);
    return Type(returnType, transformTypeAstVector(params_, typeContext), errorType);
}

}  // namespace EmojicodeCompiler
