//
//  AbstractParser.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "AbstractParser.hpp"
#include "Procedure.hpp"
#include "utf8.h"
#include "Protocol.hpp"
#include "TypeContext.hpp"

const Token& AbstractParser::parseTypeName(EmojicodeChar *typeName, EmojicodeChar *enamespace, bool *optional) {
    if (stream_.nextTokenIs(VARIABLE)) {
        throw CompilerErrorException(stream_.consumeToken(VARIABLE), "Generic variables not allowed here.");
    }
    
    *optional = stream_.nextTokenIs(E_CANDY) ? stream_.consumeToken(IDENTIFIER), true : false;

    if (stream_.nextTokenIs(E_ORANGE_TRIANGLE)) {
        *enamespace = stream_.consumeToken(IDENTIFIER).value[0];
    }
    else {
        *enamespace = globalNamespace;
    }
    
    auto &className = stream_.consumeToken(IDENTIFIER);
    *typeName = className.value[0];
    return className;
}

Type AbstractParser::parseAndFetchType(TypeContext ct, TypeDynamism dynamism, TypeDynamism *dynamicType,
                                       bool allowProtocolsUsingSelf) {
    bool optional = false;
    if (stream_.nextTokenIs(E_CANDY)) {
        stream_.consumeToken(IDENTIFIER);
        optional = true;
    }
    
    if (dynamism & GenericTypeVariables && (ct.calleeType().canHaveGenericArguments()|| ct.procedure()) &&
        stream_.nextTokenIs(VARIABLE)) {
        if (dynamicType) *dynamicType = GenericTypeVariables;
        
        auto &variableToken = stream_.consumeToken(VARIABLE);
        
        if (ct.procedure()) {
            auto it = ct.procedure()->genericArgumentVariables.find(variableToken.value);
            if (it != ct.procedure()->genericArgumentVariables.end()) {
                Type type = it->second;
                if (optional) type.setOptional();
                return type;
            }
        }
        if (ct.calleeType().canHaveGenericArguments()) {
            Type type = typeNothingness;
            if (ct.calleeType().typeDefinitionWithGenerics()->fetchVariable(variableToken.value, optional, &type)) {
                return type;
            }
        }
        
        throw CompilerErrorException(variableToken, "No such generic type variable \"%s\".", variableToken.value.utf8CString());
    }
    else if (stream_.nextTokenIs(E_ROOTSTER)) {
        auto &ratToken = stream_.consumeToken(IDENTIFIER);
        if (!(dynamism & Self)) throw CompilerErrorException(ratToken, "🐓 not allowed here.");
        if (dynamicType) *dynamicType = Self;
        return Type(TT_SELF, optional);
    }
    else if (stream_.nextTokenIs(E_GRAPES)) {
        stream_.consumeToken(IDENTIFIER);
        if (dynamicType) *dynamicType = NoDynamism;

        Type t(TT_CALLABLE, optional);
        t.arguments = 0;
        t.genericArguments.push_back(typeNothingness);
        
        while (stream_.nextTokenIsEverythingBut(E_WATERMELON) && stream_.nextTokenIsEverythingBut(E_RIGHTWARDS_ARROW)) {
            t.arguments++;
            t.genericArguments.push_back(parseAndFetchType(ct, dynamism));
        }
        
        if (stream_.nextTokenIs(E_RIGHTWARDS_ARROW)) {
            stream_.consumeToken(IDENTIFIER);
            t.genericArguments[0] = parseAndFetchType(ct, dynamism);
        }
        
        auto &token = stream_.consumeToken(IDENTIFIER);
        if (token.value[0] != E_WATERMELON) {
            throw CompilerErrorException(token, "Expected 🍉.");
        }
        
        return t;
    }
    else {
        if (dynamicType) *dynamicType = NoDynamism;
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
        
        if (!allowProtocolsUsingSelf && type.type() == TT_PROTOCOL && type.protocol->usesSelf()) {
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
        auto typeDef = type->typeDefinitionWithGenerics();
        auto offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        type->genericArguments = std::vector<Type>(typeDef->superGenericArguments());
        
        if (typeDef->numberOfOwnGenericArguments()) {
            int count = 0;
            while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
                auto token = stream_.consumeToken(IDENTIFIER);
                
                Type ta = parseAndFetchType(ct, dynamism, nullptr);
                
                auto i = count + offset;
                if (typeDef->numberOfGenericArgumentsWithSuperArguments() <= i) {
                    auto name = type->toString(ct, true);
                    throw CompilerErrorException(token, "Too many generic arguments provided for %s.", name.c_str());
                }
                if (!ta.compatibleTo(typeDef->genericArgumentConstraints()[i], ct)) {
                    auto thisName = typeDef->genericArgumentConstraints()[i].toString(ct, true);
                    auto thatName = ta.toString(ct, true);
                    throw CompilerErrorException(token, "Generic argument %s is not compatible to constraint %s.",
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

bool AbstractParser::parseArgumentList(Callable *c, TypeContext ct) {
    bool usedSelf = false;
    
    while (stream_.nextTokenIs(VARIABLE)) {
        TypeDynamism dynamism;
        auto &variableToken = stream_.consumeToken(VARIABLE);
        auto type = parseAndFetchType(ct, AllKindsOfDynamism, &dynamism);
        
        c->arguments.push_back(Argument(variableToken, type));
        
        if (dynamism == Self) {
            usedSelf = true;
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
        c->returnType = parseAndFetchType(ct, AllKindsOfDynamism, &dynamism);
        if (dynamism == Self) {
            usedSelf = true;
        }
    }
    return usedSelf;
}

void AbstractParser::parseGenericArgumentsInDefinition(Procedure *p, TypeContext ct) {
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken();
        auto &variable = stream_.consumeToken(VARIABLE);
        
        Type t = parseAndFetchType(Type(p->eclass), GenericTypeVariables, nullptr, true);
        p->genericArgumentConstraints.push_back(t);
        
        Type rType = Type(TT_LOCAL_REFERENCE, false);
        rType.reference = p->genericArgumentVariables.size();
        
        if (p->genericArgumentVariables.count(variable.value)) {
            throw CompilerErrorException(variable, "A generic argument variable with the same name is already in use.");
        }
        p->genericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable.value, rType));
    }
}

void AbstractParser::parseBody(Procedure *p, bool allowNative) {
    if (stream_.nextTokenIs(E_RADIO)) {
        auto &radio = stream_.consumeToken(IDENTIFIER);
        if (!allowNative) {
            throw CompilerErrorException(radio, "Native code is not allowed in this context.");
        }
        p->native = true;
    }
    else {
        auto &token = stream_.consumeToken(IDENTIFIER);
        
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
        if (token.type() == IDENTIFIER && token.value[0] == E_GRAPES) {
            depth++;
        }
        else if (token.type() == IDENTIFIER && token.value[0] == E_WATERMELON) {
            if (depth == 0) {
                break;
            }
            depth--;
        }
    }
}
