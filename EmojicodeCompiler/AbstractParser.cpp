//
//  AbstractParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "AbstractParser.hpp"
#include "Function.hpp"
#include "utf8.h"
#include "Protocol.hpp"
#include "TypeContext.hpp"

const Token& AbstractParser::parseTypeName(EmojicodeChar *typeName, EmojicodeChar *enamespace, bool *optional) {
    if (stream_.nextTokenIs(TokenType::Variable)) {
        throw CompilerErrorException(stream_.consumeToken(TokenType::Variable), "Generic variables not allowed here.");
    }
    
    *optional = stream_.nextTokenIs(E_CANDY) ? stream_.consumeToken(TokenType::Identifier), true : false;

    if (stream_.nextTokenIs(E_ORANGE_TRIANGLE)) {
        stream_.consumeToken();
        *enamespace = stream_.consumeToken(TokenType::Identifier).value[0];
    }
    else {
        *enamespace = globalNamespace;
    }
    
    auto &className = stream_.consumeToken(TokenType::Identifier);
    *typeName = className.value[0];
    return className;
}

Type AbstractParser::parseTypeDeclarative(TypeContext ct, TypeDynamism dynamism, Type expectation,
                                       TypeDynamism *dynamicType, bool allowProtocolsUsingSelf) {
    if (stream_.nextTokenIs(E_MEDIUM_BLACK_CIRCLE)) {
        if (expectation.type() == TypeContent::Nothingness) {
            throw CompilerErrorException(stream_.consumeToken(), "Cannot infer ⚫️.");
        }
        stream_.consumeToken();
        return expectation.copyWithoutOptional();
    }
    if (stream_.nextTokenIs(E_WHITE_SQUARE_BUTTON)) {
        auto &token = stream_.consumeToken();
        Type type = parseTypeDeclarative(ct, dynamism, expectation, dynamicType, allowProtocolsUsingSelf);
        if (!type.allowsMetaType() || type.meta()) {
            throw CompilerErrorException(token, "Meta type of %s is restricted.", type.toString(ct, true).c_str());
        }
        type.setMeta(true);
        return type;
    }
    
    bool optional = false;
    if (stream_.nextTokenIs(E_CANDY)) {
        stream_.consumeToken(TokenType::Identifier);
        optional = true;
    }
    
    if ((dynamism & TypeDynamism::GenericTypeVariables) != TypeDynamism::None && (ct.calleeType().canHaveGenericArguments()|| ct.function()) &&
        stream_.nextTokenIs(TokenType::Variable)) {
        if (dynamicType) *dynamicType = TypeDynamism::GenericTypeVariables;
        
        auto &variableToken = stream_.consumeToken(TokenType::Variable);
        
        if (ct.function()) {
            auto it = ct.function()->genericArgumentVariables.find(variableToken.value);
            if (it != ct.function()->genericArgumentVariables.end()) {
                Type type = it->second;
                if (optional) type.setOptional();
                return type;
            }
        }
        if (ct.calleeType().canHaveGenericArguments()) {
            Type type = typeNothingness;
            if (ct.calleeType().typeDefinitionFunctional()->fetchVariable(variableToken.value, optional, &type)) {
                return type;
            }
        }
        
        throw CompilerErrorException(variableToken, "No such generic type variable \"%s\".", variableToken.value.utf8CString());
    }
    else if (stream_.nextTokenIs(E_DOG)) {
        auto &ratToken = stream_.consumeToken(TokenType::Identifier);
        if ((dynamism & TypeDynamism::Self) == TypeDynamism::None) {
            throw CompilerErrorException(ratToken, "🐕 not allowed here.");
        }
        if (dynamicType) *dynamicType = TypeDynamism::Self;
        return Type(TypeContent::Self, optional);
    }
    else if (stream_.nextTokenIs(E_GRAPES)) {
        stream_.consumeToken(TokenType::Identifier);
        if (dynamicType) *dynamicType = TypeDynamism::None;

        Type t(TypeContent::Callable, optional);
        t.genericArguments.push_back(typeNothingness);
        
        while (stream_.nextTokenIsEverythingBut(E_WATERMELON) && stream_.nextTokenIsEverythingBut(E_RIGHTWARDS_ARROW)) {
            t.genericArguments.push_back(parseTypeDeclarative(ct, dynamism));
        }
        
        if (stream_.nextTokenIs(E_RIGHTWARDS_ARROW)) {
            stream_.consumeToken(TokenType::Identifier);
            t.genericArguments[0] = parseTypeDeclarative(ct, dynamism);
        }
        
        auto &token = stream_.consumeToken(TokenType::Identifier);
        if (token.value[0] != E_WATERMELON) {
            throw CompilerErrorException(token, "Expected 🍉.");
        }
        
        return t;
    }
    else {
        if (dynamicType) *dynamicType = TypeDynamism::None;
        EmojicodeChar typeName, typeNamespace;
        bool optionalUseless;
        auto &token = parseTypeName(&typeName, &typeNamespace, &optionalUseless);
        
        auto type = typeNothingness;
        if (!package_->fetchRawType(typeName, typeNamespace, optional, token, &type)) {
            ecCharToCharStack(typeName, nameString);
            ecCharToCharStack(typeNamespace, namespaceString);
            throw CompilerErrorException(token, "Could not find type %s in enamespace %s.", nameString, namespaceString);
        }
        
        parseGenericArgumentsForType(&type, ct, dynamism, token);
        
        if (!allowProtocolsUsingSelf && type.type() == TypeContent::Protocol && type.protocol()->usesSelf()) {
            auto typeStr = type.toString(ct, true);
            throw CompilerErrorException(token, "Protocol %s can only be used as a generic constraint because it uses 🐓.",
                          typeStr.c_str());
        }
        
        return type;
    }
}

void AbstractParser::parseGenericArgumentsForType(Type *type, TypeContext ct, TypeDynamism dynamism,
                                                  const Token& errorToken) {
    if (type->canHaveGenericArguments()) {
        auto typeDef = type->typeDefinitionFunctional();
        auto offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        type->genericArguments = std::vector<Type>(typeDef->superGenericArguments());
        
        if (typeDef->numberOfOwnGenericArguments()) {
            int count = 0;
            while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
                auto token = stream_.consumeToken(TokenType::Identifier);
                
                Type ta = parseTypeDeclarative(ct, dynamism, typeNothingness, nullptr);
                
                auto i = count + offset;
                if (typeDef->numberOfGenericArgumentsWithSuperArguments() <= i) {
                    auto name = type->toString(ct, true);
                    throw CompilerErrorException(token, "Too many generic arguments provided for %s.", name.c_str());
                }
                if (!ta.compatibleTo(typeDef->genericArgumentConstraints()[i], ct)) {
                    auto thisName = typeDef->genericArgumentConstraints()[i].toString(ct, true);
                    auto thatName = ta.toString(ct, true);
                    throw CompilerErrorException(token, "Generic argument for %s is not compatible to constraint %s.",
                                  thatName.c_str(), thisName.c_str());
                }
                
                type->genericArguments.push_back(ta);
                
                count++;
            }
            
            if (count != typeDef->numberOfOwnGenericArguments()) {
                auto str = type->toString(typeNothingness, true);
                throw CompilerErrorException(errorToken, "Type %s requires %d generic arguments, but %d were given.",
                              str.c_str(), typeDef->numberOfOwnGenericArguments(), count);
            }
        }
    }
}

bool AbstractParser::parseArgumentList(Callable *c, TypeContext ct, bool initializer) {
    bool usedSelf = false;
    bool argumentToVariable;
    
    while ((argumentToVariable = stream_.nextTokenIs(E_BABY_BOTTLE)) || stream_.nextTokenIs(TokenType::Variable)) {
        if (argumentToVariable) {
            auto &token = stream_.consumeToken(TokenType::Identifier);
            if (!initializer) {
                throw CompilerErrorException(token, "🍼 can only be used with initializers.");
            }
        }
            
        TypeDynamism dynamism;
        auto &variableToken = stream_.consumeToken(TokenType::Variable);
        auto type = parseTypeDeclarative(ct, TypeDynamism::AllKinds, typeNothingness, &dynamism);
        
        c->arguments.push_back(Argument(variableToken, type));
        
        if (dynamism == TypeDynamism::Self) {
            usedSelf = true;
        }
        if (argumentToVariable) {
            static_cast<Initializer *>(c)->addArgumentToVariable(variableToken.value);
        }
    }
    
    if (c->arguments.size() > UINT8_MAX) {
        throw CompilerErrorException(c->position(), "A function cannot take more than 255 arguments.");
    }
    return usedSelf;
}

bool AbstractParser::parseReturnType(Callable *c, TypeContext ct) {
    bool usedSelf = false;
    if (stream_.nextTokenIs(E_RIGHTWARDS_ARROW)) {
        stream_.consumeToken();
        TypeDynamism dynamism;

        c->returnType = parseTypeDeclarative(ct, TypeDynamism::AllKinds, typeNothingness, &dynamism);
        if (dynamism == TypeDynamism::Self) {
            usedSelf = true;
        }
    }
    return usedSelf;
}

void AbstractParser::parseGenericArgumentsInDefinition(Function *p, TypeContext ct) {
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken();
        auto &variable = stream_.consumeToken(TokenType::Variable);
        
        Type t = parseTypeDeclarative(p->owningType(), TypeDynamism::GenericTypeVariables, typeNothingness, nullptr,
                                      true);
        p->genericArgumentConstraints.push_back(t);
        
        Type rType = Type(TypeContent::LocalReference, false);
        rType.reference = p->genericArgumentVariables.size();
        
        if (p->genericArgumentVariables.count(variable.value)) {
            throw CompilerErrorException(variable, "A generic argument variable with the same name is already in use.");
        }
        p->genericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable.value, rType));
    }
}

void AbstractParser::parseBody(Function *p, bool allowNative) {
    if (stream_.nextTokenIs(E_RADIO)) {
        auto &radio = stream_.consumeToken(TokenType::Identifier);
        if (!allowNative) {
            throw CompilerErrorException(radio, "Native code is not allowed in this context.");
        }
        p->native = true;
    }
    else {
        auto &token = stream_.consumeToken(TokenType::Identifier);
        
        if (token.value[0] != E_GRAPES) {
            ecCharToCharStack(token.value[0], c);
            throw CompilerErrorException(token, "Expected 🍇 but found %s instead.", c);
        }
        parseBody(p);
    }
}

void AbstractParser::parseBody(Callable *p) {
    p->setTokenStream(stream_);
    
    int depth = 0;
    while (true) {
        auto &token = stream_.consumeToken();
        if (token.type() == TokenType::Identifier && token.value[0] == E_GRAPES) {
            depth++;
        }
        else if (token.type() == TokenType::Identifier && token.value[0] == E_WATERMELON) {
            if (depth == 0) {
                break;
            }
            depth--;
        }
    }
}
