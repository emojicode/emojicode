//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <vector>
#include "utf8.h"
#include "Lexer.hpp"
#include "Class.hpp"
#include "Procedure.hpp"
#include "EmojicodeCompiler.hpp"

//MARK: Globals
/* Very important one time declarations */

Class *CL_STRING;
Class *CL_LIST;
Class *CL_ERROR;
Class *CL_DATA;
Class *CL_DICTIONARY;
Protocol *PR_ENUMERATEABLE;
Protocol *PR_ENUMERATOR;
Class *CL_RANGE;

bool Type::canHaveGenericArguments() const {
    return type() == TT_CLASS || type() == TT_PROTOCOL;
}

TypeDefinitionWithGenerics* Type::typeDefinitionWithGenerics() const {
    return static_cast<TypeDefinitionWithGenerics *>(eclass);
}

Type Type::copyWithoutOptional() const {
    Type type = *this;
    type.optional_ = false;
    return type;
}

Type Type::resolveOnSuperArgumentsAndConstraints(TypeContext typeContext, bool resolveSelf) const {
    TypeDefinitionWithGenerics *c = typeContext.normalType.typeDefinitionWithGenerics();
    Type t = *this;
    bool optional = t.optional();
    
    if (resolveSelf && t.type() == TT_SELF) {
        t = typeContext.normalType;
    }
    
    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->numberOfOwnGenericArguments();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TT_REFERENCE && t.reference < maxReferenceForSuper) {
        t = c->superGenericArguments()[t.reference];
    }
    while (t.type() == TT_LOCAL_REFERENCE) {
        t = typeContext.p->genericArgumentConstraints[t.reference];
    }
    while (t.type() == TT_REFERENCE) {
        t = typeContext.normalType.typeDefinitionWithGenerics()->genericArgumentConstraints()[t.reference];
    }
    
    if (optional) t.setOptional();
    return t;
}

Type Type::resolveOn(TypeContext typeContext, bool resolveSelf) const {
    Type t = *this;
    bool optional = t.optional();
    
    if (resolveSelf && t.type() == TT_SELF) {
        t = typeContext.normalType;
    }
    
    while (t.type() == TT_LOCAL_REFERENCE) {
        t = (*typeContext.procedureGenericArguments)[t.reference];
    }
    
    if (typeContext.normalType.canHaveGenericArguments()) {
        while (t.type() == TT_REFERENCE &&
               typeContext.normalType.typeDefinitionWithGenerics()->canBeUsedToResolve(t.resolutionConstraint)) {
            Type tn = typeContext.normalType.genericArguments[t.reference];
            if (tn.type() == TT_REFERENCE && tn.reference == t.reference) {
                break;
            }
            t = tn;
        }
    }
    
    if (optional) t.setOptional();
    
    if (t.canHaveGenericArguments()) {
        for (int i = 0; i < t.typeDefinitionWithGenerics()->numberOfGenericArgumentsWithSuperArguments(); i++) {
            t.genericArguments[i] = t.genericArguments[i].resolveOn(typeContext);
        }
    }
    else if (t.type() == TT_CALLABLE) {
        for (int i = 0; i < t.arguments + 1; i++) {
            t.genericArguments[i] = t.genericArguments[i].resolveOn(typeContext);
        }
    }
    
    return t;
}

/** Returns the name of a type */

bool Type::compatibleTo(Type to, TypeContext ct) const {
    //(to.optional || !a.optional): Either `to` accepts optionals, or if `to` does not accept optionals `a` mustn't be one.
    if (to.type() == TT_SOMETHING) {
        return true;
    }
    else if (to.type() == TT_SOMEOBJECT && (this->type() == TT_CLASS ||
                                            this->type() == TT_PROTOCOL || this->type() == TT_SOMEOBJECT)) {
        return to.optional() || !this->optional();
    }
    else if (this->type() == TT_CLASS && to.type() == TT_CLASS) {
        if ((to.optional() || !this->optional()) && this->eclass->inheritsFrom(to.eclass)) {
            if (to.eclass->numberOfOwnGenericArguments()) {
                for (int l = to.eclass->numberOfOwnGenericArguments(), i = to.eclass->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
                    if (!this->genericArguments[i].identicalTo(to.genericArguments[i])) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    else if (this->type() == TT_PROTOCOL && to.type() == TT_PROTOCOL) {
        if ((to.optional() || !this->optional()) && this->protocol == to.protocol) {
            if (to.eclass->numberOfOwnGenericArguments()) {
                for (int l = to.eclass->numberOfOwnGenericArguments(), i = to.eclass->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
                    if (!this->genericArguments[i].identicalTo(to.genericArguments[i])) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    else if (this->type() == TT_CLASS && to.type() == TT_PROTOCOL) {
        if (to.optional() || !this->optional()) {
            for (Class *a = this->eclass; a != nullptr; a = a->superclass) {
                for (auto protocol : a->protocols()) {
                    if (protocol.resolveOn(*this).compatibleTo(to, ct)) return true;
                }
            }
        }
        return false;
    }
    else if (this->type() == TT_NOTHINGNESS) {
        return to.optional() || to.type() == TT_NOTHINGNESS;
    }
    else if (this->type() == TT_ENUM && to.type() == TT_ENUM) {
        return (to.optional() || !this->optional()) && this->eenum == to.eenum;
    }
    else if ((this->type() == TT_REFERENCE && to.type() == TT_REFERENCE) ||
             (this->type() == TT_LOCAL_REFERENCE && to.type() == TT_LOCAL_REFERENCE)) {
        if ((to.optional() || !this->optional()) && this->reference == to.reference) {
            return true;
        }
        return (to.optional() || !this->optional())
        && this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct);
    }
    else if (this->type() == TT_REFERENCE) {
        return (to.optional() || !this->optional()) && this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct);
    }
    else if (to.type() == TT_REFERENCE) {
        return (to.optional() || !this->optional()) && this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct);
    }
    else if (this->type() == TT_LOCAL_REFERENCE) {
        return (to.optional() || !this->optional()) && this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct);
    }
    else if (to.type() == TT_LOCAL_REFERENCE) {
        return (to.optional() || !this->optional()) && this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct);
    }
    else if (to.type() == TT_SELF) {
        return (to.optional() || !this->optional()) && this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct);
    }
    else if (this->type() == TT_SELF) {
        return (to.optional() || !this->optional()) && this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct);
    }
    else if (this->type() == TT_CALLABLE && to.type() == TT_CALLABLE) {
        if (this->genericArguments[0].compatibleTo(to.genericArguments[0], ct) && to.arguments == this->arguments) {
            for (int i = 1; i <= to.arguments; i++) {
                if (!to.genericArguments[i].compatibleTo(this->genericArguments[i], ct)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    else {
        return (to.optional() || !this->optional()) && this->type() == to.type();
    }
    return false;
}

bool Type::identicalTo(Type to) const {
    if (type() == to.type()) {
        switch (type()) {
            case TT_CLASS:
                if (eclass == to.eclass) {
                    if (to.eclass->numberOfOwnGenericArguments()) {
                        for (int l = to.eclass->numberOfOwnGenericArguments(), i = to.eclass->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
                            if (!this->genericArguments[i].identicalTo(to.genericArguments[i])) {
                                return false;
                            }
                        }
                    }
                    return true;
                }
                return false;
            case TT_CALLABLE:
                if (this->genericArguments[0].identicalTo(to.genericArguments[0]) && to.arguments == this->arguments) {
                    for (int i = 1; i <= to.arguments; i++) {
                        if (!to.genericArguments[i].identicalTo(this->genericArguments[i])) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            case TT_ENUM:
                return eenum == to.eenum;
            case TT_PROTOCOL:
                return protocol == to.protocol;
            case TT_REFERENCE:
            case TT_LOCAL_REFERENCE:
                return reference == to.reference;
            case TT_SELF:
            case TT_DOUBLE:
            case TT_INTEGER:
            case TT_SYMBOL:
            case TT_SOMETHING:
            case TT_BOOLEAN:
            case TT_SOMEOBJECT:
            case TT_NOTHINGNESS:
                return true;
        }
    }
    return false;
}

//MARK: Type Parsing Utility

const Token* Type::parseTypeName(EmojicodeChar *typeName, EmojicodeChar *enamespace, bool *optional) {
    if (nextToken()->type == VARIABLE) {
        compilerError(consumeToken(), "Generic variables not allowed here.");
    }
    auto *className = consumeToken(IDENTIFIER);
    
    if (className->value[0] == E_CANDY) {
        *optional = true;
        
        className = consumeToken(IDENTIFIER);
    }
    else {
        *optional = false;
    }
    
    if (className->value[0] == E_ORANGE_TRIANGLE) {
        const Token *nsToken = consumeToken(IDENTIFIER);
        *enamespace = nsToken->value[0];
        
        className = consumeToken(IDENTIFIER);
    }
    else {
        *enamespace = globalNamespace;
    }
    
    *typeName = className->value[0];
    
    return className;
}

Type Type::parseAndFetchType(TypeContext ct, TypeDynamism dynamism, Package *package, TypeDynamism *dynamicType,
                             bool allowProtocolsUsingSelf) {
    auto beforeCandy = currentToken;
    bool optional = false;
    if (nextToken()->value[0] == E_CANDY) {
        consumeToken();
        optional = true;
    }
    
    if (dynamism & GenericTypeVariables && (ct.normalType.canHaveGenericArguments()|| ct.p) &&
        nextToken()->type == VARIABLE) {
        if (dynamicType) *dynamicType = GenericTypeVariables;
        
        auto variableToken = consumeToken();
        
        if (ct.p) {
            auto it = ct.p->genericArgumentVariables.find(variableToken->value);
            if (it != ct.p->genericArgumentVariables.end()) {
                Type type = it->second;
                if (optional) type.setOptional();
                return type;
            }
        }
        if (ct.normalType.canHaveGenericArguments()) {
            Type type = typeNothingness;
            if (ct.normalType.typeDefinitionWithGenerics()->fetchVariable(variableToken->value, optional, &type)) {
                return type;
            }
        }
            
        compilerError(variableToken, "No such generic type variable \"%s\".", variableToken->value.utf8CString());
    }
    else if (nextToken()->value[0] == E_ROOTSTER) {
        auto ratToken = consumeToken();
        if (!(dynamism & Self)) compilerError(ratToken, "🐓 not allowed here.");
        if (dynamicType) *dynamicType = Self;
        return Type(TT_SELF, optional);
    }
    else if (nextToken()->value[0] == E_GRAPES) {
        if (dynamicType) *dynamicType = NoDynamism;
        consumeToken();
        
        Type t(TT_CALLABLE, optional);
        t.arguments = 0;
        
        t.genericArguments.push_back(typeNothingness);
        
        while (!(nextToken()->type == IDENTIFIER && (nextToken()->value[0] == E_WATERMELON ||
                                                     nextToken()->value[0] == E_RIGHTWARDS_ARROW))) {
            t.arguments++;
            t.genericArguments.push_back(parseAndFetchType(ct, dynamism, package));
        }
        
        if (nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW) {
            consumeToken();
            t.genericArguments[0] = parseAndFetchType(ct, dynamism, package);
        }
        
        const Token *token = consumeToken(IDENTIFIER);
        if (token->value[0] != E_WATERMELON) {
            compilerError(token, "Expected 🍉.");
        }
        
        return t;
    }
    else {
        if (dynamicType) *dynamicType = NoDynamism;
        currentToken = beforeCandy;
        EmojicodeChar typeName, typeNamespace;
        bool optional;
        const Token *token = parseTypeName(&typeName, &typeNamespace, &optional);
        
        auto type = typeNothingness;
        if (!package->fetchRawType(typeName, typeNamespace, optional, token, &type)) {
            ecCharToCharStack(typeName, nameString);
            ecCharToCharStack(typeNamespace, namespaceString);
            compilerError(token, "Could not find type %s in enamespace %s.", nameString, namespaceString);
        }
        
        type.parseGenericArguments(ct, dynamism, package, token);
        
        if (!allowProtocolsUsingSelf && type.type() == TT_PROTOCOL && type.protocol->usesSelf()) {
            auto typeStr = type.toString(ct, true);
            compilerError(token, "Protocol %s can only be used as a generic constraint because it uses 🐓.",
                          typeStr.c_str());
        }
        
        return type;
    }
}

void Type::parseGenericArguments(TypeContext ct, TypeDynamism dynamism, Package *package, const Token *errorToken) {
    if (canHaveGenericArguments()) {
        auto typeDef = typeDefinitionWithGenerics();
        auto offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        genericArguments = std::vector<Type>(typeDef->superGenericArguments());
        
        if (typeDef->numberOfOwnGenericArguments()) {
            int count = 0;
            while (nextToken()->value[0] == E_SPIRAL_SHELL) {
                auto token = consumeToken();
                
                Type ta = parseAndFetchType(ct, dynamism, package, nullptr);
                
                auto i = count + offset;
                if (typeDef->numberOfGenericArgumentsWithSuperArguments() <= i) {
                    auto name = toString(ct, true);
                    compilerError(token, "Too many generic arguments provided for %s.", name.c_str());
                }
                if (!ta.compatibleTo(typeDef->genericArgumentConstraints()[i], ct)) {
                    auto thisName = typeDef->genericArgumentConstraints()[i].toString(ct, true);
                    auto thatName = ta.toString(ct, true);
                    compilerError(token, "Generic argument %s is not compatible to constraint %s.",
                                  thatName.c_str(), thisName.c_str());
                }
                
                genericArguments.push_back(ta);
                
                count++;
            }
            
            if (count != typeDef->numberOfOwnGenericArguments()) {
                auto str = toString(typeNothingness, true);
                compilerError(errorToken, "Type %s requires %d generic arguments, but %d were given.",
                              str.c_str(), typeDef->numberOfOwnGenericArguments(), count);
            }
        }
    }
}

//MARK: Type Interferring

void CommonTypeFinder::addType(Type t, TypeContext typeContext) {
    if (!firstTypeFound) {
        commonType = t;
        firstTypeFound = true;
    }
    else if (!t.compatibleTo(commonType, typeContext)) {
        if (commonType.compatibleTo(t, typeContext)) {
            commonType = t;
        }
        else if (t.type() == TT_CLASS && commonType.type() == TT_CLASS) {
            commonType = typeSomeobject;
        }
        else {
            commonType = typeSomething;
        }
    }
}

Type CommonTypeFinder::getCommonType(const Token *warningToken) {
    if (!firstTypeFound) {
        compilerWarning(warningToken, "Type is ambigious without more context.");
    }
    return commonType;
}

//MARK: Type Visulisation

const char* Type::typePackage() {
    switch (this->type()) {
        case TT_CLASS:
            return this->eclass->package()->name();
        case TT_PROTOCOL:
            return this->protocol->package()->name();
        case TT_ENUM:
            return this->eenum->package()->name();
        case TT_INTEGER:
        case TT_NOTHINGNESS:
        case TT_BOOLEAN:
        case TT_SYMBOL:
        case TT_DOUBLE:
        case TT_SOMETHING:
        case TT_SOMEOBJECT:
        case TT_REFERENCE:
        case TT_LOCAL_REFERENCE:
        case TT_CALLABLE:
        case TT_SELF:
            return "";
    }
}

void stringAppendEc(EmojicodeChar c, std::string &string) {
    ecCharToCharStack(c, sc);
    string.append(sc);
}

Type::Type(Class *c, bool o) : optional_(o), type_(TT_CLASS), eclass(c) {
    for (int i = 0; i < eclass->numberOfGenericArgumentsWithSuperArguments(); i++) {
        genericArguments.push_back(Type(TT_REFERENCE, false, i, c));
    }
}

void Type::typeName(Type type, TypeContext typeContext, bool includePackageAndOptional, std::string &string) const {
    if (includePackageAndOptional) {
        if (type.optional()) {
            stringAppendEc(E_CANDY, string);
        }
        
        string.append(type.typePackage());
    }
    
    switch (type.type()) {
        case TT_CLASS:
            stringAppendEc(type.eclass->name(), string);
            break;
        case TT_PROTOCOL:
            stringAppendEc(type.protocol->name(), string);
            break;
        case TT_ENUM:
            stringAppendEc(type.eenum->name(), string);
            return;
        case TT_INTEGER:
            stringAppendEc(E_STEAM_LOCOMOTIVE, string);
            return;
        case TT_NOTHINGNESS:
            stringAppendEc(E_SPARKLES, string);
            return;
        case TT_BOOLEAN:
            stringAppendEc(E_OK_HAND_SIGN, string);
            return;
        case TT_SYMBOL:
            stringAppendEc(E_INPUT_SYMBOL_FOR_SYMBOLS, string);
            return;
        case TT_DOUBLE:
            stringAppendEc(E_ROCKET, string);
            return;
        case TT_SOMETHING:
            stringAppendEc(E_MEDIUM_WHITE_CIRCLE, string);
            return;
        case TT_SOMEOBJECT:
            stringAppendEc(E_LARGE_BLUE_CIRCLE, string);
            return;
        case TT_SELF:
            stringAppendEc(E_ROOTSTER, string);
            return;
        case TT_CALLABLE:
            stringAppendEc(E_GRAPES, string);
            
            for (int i = 1; i <= type.arguments; i++) {
                typeName(type.genericArguments[i], typeContext, includePackageAndOptional, string);
            }
            
            stringAppendEc(E_RIGHTWARDS_ARROW, string);
            stringAppendEc(0xFE0F, string);
            typeName(type.genericArguments[0], typeContext, includePackageAndOptional, string);
            stringAppendEc(E_WATERMELON, string);
            return;
        case TT_REFERENCE: {
            if (typeContext.normalType.type() == TT_CLASS) {
                Class *eclass = typeContext.normalType.eclass;
                do {
                    for (auto it : eclass->ownGenericArgumentVariables()) {
                        if (it.second.reference == type.reference) {
                            string.append(it.first.utf8CString());
                            return;
                        }
                    }
                } while ((eclass = eclass->superclass));
            }
            else if (typeContext.normalType.canHaveGenericArguments()) {
                for (auto it : typeContext.normalType.typeDefinitionWithGenerics()->ownGenericArgumentVariables()) {
                    if (it.second.reference == type.reference) {
                        string.append(it.first.utf8CString());
                        return;
                    }
                }
            }
            
            stringAppendEc('T', string);
            stringAppendEc('0' + type.reference, string);
            return;
        }
        case TT_LOCAL_REFERENCE:
            if (typeContext.p) {
                for (auto it : typeContext.p->genericArgumentVariables) {
                    if (it.second.reference == type.reference) {
                        string.append(it.first.utf8CString());
                        return;
                    }
                }
            }
            
            stringAppendEc('L', string);
            stringAppendEc('0' + type.reference, string);
            return;
    }
    
    if (typeContext.normalType.type() != TT_NOTHINGNESS && type.canHaveGenericArguments()) {
        auto typeDef = type.typeDefinitionWithGenerics();
        int offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        for (int i = 0, l = typeDef->numberOfOwnGenericArguments(); i < l; i++) {
            stringAppendEc(E_SPIRAL_SHELL, string);
            typeName(type.genericArguments[offset + i], typeContext, includePackageAndOptional, string);
        }
    }
}

std::string Type::toString(TypeContext typeContext, bool includeNsAndOptional) const {
    std::string string;
    typeName(*this, typeContext, includeNsAndOptional, string);
    return string;
}
