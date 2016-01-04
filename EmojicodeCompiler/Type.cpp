//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.h"
#include "utf8.h"
#include <string.h>

//MARK: Globals
/* Very important one time declarations */

Class *CL_STRING;

Class *CL_LIST;

Class *CL_ERROR;

Class *CL_DATA;

Class *CL_DICTIONARY;

Protocol *PR_ENUMERATEABLE;

Class *CL_ENUMERATOR;

Type Type::typeConstraintForReference(Class *c){
    Type t = *this;
    bool optional = t.optional;
    while (t.type == TT_REFERENCE) {
        t = c->genericArgumentContraints[t.reference];
    }
    t.optional = optional;
    return t;
}

Type Type::resolveOnSuperArguments(Class *c, bool *resolved){
    Type t = *this;
    while (true) {
        if (t.type != TT_REFERENCE || !c->superclass || t.reference >= c->superclass->genericArgumentCount) {
            return t;
        }
        *resolved = true;
        t = c->superGenericArguments[t.reference];
    }
}

/** Returns the name of a type */

bool Type::compatibleTo(Type to, Type parentType){
    //(to.optional || !a.optional): Either `to` accepts optionals, or if `to` does not accept optionals `a` mustn't be one.
    if (to.type == TT_SOMETHING) {
        return true;
    }
    else if(to.type == TT_SOMEOBJECT && (this->type == TT_CLASS || this->type == TT_PROTOCOL || this->type == TT_SOMEOBJECT)){
        return to.optional || !this->optional;
    }
    else if(this->type == TT_CLASS && to.type == TT_CLASS){
        if ((to.optional || !this->optional) && this->eclass->inheritsFrom(to.eclass)) {
            if (to.eclass->ownGenericArgumentCount) {
                for (int l = to.eclass->ownGenericArgumentCount, i = to.eclass->genericArgumentCount - l; i < l; i++) {
                    if (!this->genericArguments[i].compatibleTo(to.genericArguments[i], parentType)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    else if(this->type == TT_PROTOCOL && this->type == TT_PROTOCOL){
        return (to.optional || !this->optional) && this->protocol == to.protocol;
    }
    else if(this->type == TT_CLASS && to.type == TT_PROTOCOL){
        return (to.optional || !this->optional) && this->eclass->conformsTo(to.protocol);
    }
    else if(this->type == TT_NOTHINGNESS){
        return to.optional || to.type == TT_NOTHINGNESS;
    }
    else if(this->type == TT_REFERENCE && to.type == TT_REFERENCE){
        if((to.optional || !this->optional) && this->reference == to.reference) {
            return true;
        }
        return (to.optional || !this->optional) && this->typeConstraintForReference(parentType.eclass).compatibleTo(to.typeConstraintForReference(parentType.eclass), parentType);
    }
    else if(this->type == TT_ENUM && to.type == TT_ENUM) {
        return (to.optional || !this->optional) && this->eenum == to.eenum;
    }
    else if(this->type == TT_REFERENCE) {
        bool resolved = false;
        Type rt = this->resolveOnSuperArguments(parentType.eclass, &resolved);
        if(resolved && (to.optional || !this->optional) && rt.compatibleTo(to, parentType)) {
            return true;
        }
        return (to.optional || !this->optional) && this->typeConstraintForReference(parentType.eclass).compatibleTo(to, parentType);
    }
    else if(to.type == TT_REFERENCE){
        bool resolved = false;
        Type rt = to.resolveOnSuperArguments(parentType.eclass, &resolved);
        if(resolved && (to.optional || !this->optional) && this->compatibleTo(rt, parentType)) {
            return true;
        }
        return (to.optional || !this->optional) && this->compatibleTo(to.typeConstraintForReference(parentType.eclass), parentType);
    }
    else if(this->type == TT_CALLABLE && to.type == TT_CALLABLE) {
        if (typesCompatible(this->genericArguments[0], to.genericArguments[0], parentType) && to.arguments == this->arguments) {
            for (int i = 1; i <= to.arguments; i++) {
                if (!typesCompatible(to.genericArguments[i], this->genericArguments[i], parentType)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    else {
        return (to.optional || !this->optional) && this->type == to.type;
    }
    return false;
}

Type fetchRawType(EmojicodeChar name, EmojicodeChar enamespace, bool optional, Token *token, bool *existent){
    *existent = true;
    if(enamespace == globalNamespace){
        switch (name) {
            case E_OK_HAND_SIGN:
                return Type(TT_BOOLEAN, optional);
            case E_INPUT_SYMBOL_FOR_SYMBOLS:
                return Type(TT_SYMBOL, optional);
            case E_STEAM_LOCOMOTIVE:
                return Type(TT_INTEGER, optional);
            case E_ROCKET:
                return Type(TT_DOUBLE, optional);
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    compilerWarning(token, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                return Type(TT_SOMETHING, false);
            case E_LARGE_BLUE_CIRCLE:
                return Type(TT_SOMEOBJECT, optional);
            case E_SPARKLES:
                compilerError(token, "The Nothingness type may not be referenced to.");
        }
    }
    
    Class *eclass = getClass(name, enamespace);
    if (eclass) {
        Type t = typeForClass(eclass);
        t.optional = optional;
        return t;
    }
    
    Protocol *protocol = getProtocol(name, enamespace);
    if(protocol){
        return Type(protocol, optional);
    }
    
    Enum *eenum = getEnum(name, enamespace);
    if(eenum){
        return Type(eenum, optional);
    }
    
    *existent = false;
    return typeNothingness;
}

Type resolveTypeReferences(Type t, Type o){
    bool optional = t.optional;
    while (t.type == TT_REFERENCE) {
        t = o.genericArguments[t.reference];
    }
    t.optional = optional;
    if (t.type == TT_CLASS) {
        for (int i = 0; i < t.eclass->genericArgumentCount; i++) {
            t.genericArguments[i] = resolveTypeReferences(t.genericArguments[i], o);
        }
    }
    return t;
}

//MARK: Type Parsing Utility

int initializeAndCopySuperGenericArguments(Type *type){
    if (type->type == TT_CLASS) {
        type->genericArguments = new Type[type->eclass->genericArgumentCount];
        int offset = type->eclass->genericArgumentCount - type->eclass->ownGenericArgumentCount;
        
        if (type->eclass->superclass){
            memcpy(type->genericArguments, type->eclass->superGenericArguments, offset * sizeof(Type));
        }
        
        if (type->eclass->ownGenericArgumentCount){
            return offset;
        }
    }
    return -1;
}

void Type::validateGenericArgument(Type ta, uint16_t i, Token *token){
    if(this->eclass->superclass){
        i += this->eclass->superclass->genericArgumentCount;
    }
    if(!ta.compatibleTo(this->eclass->genericArgumentContraints[i], *this)){
        compilerError(token, "Types not matching.");
    }
}

void checkEnoughGenericArguments(uint16_t count, Type type, Token *token){
    if(count != type.eclass->ownGenericArgumentCount){
        char *typeString = typeToString(type, typeNothingness, true);
        compilerError(token, "Type %s requires %d generic arguments, but %d were given.", typeString, type.eclass->ownGenericArgumentCount, count);
    }
}

Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *enamespace, bool *optional, EmojicodeChar currentNamespace){
    Token *className = consumeToken();
    if (className->type == VARIABLE) {
        compilerError(className, "Not in a generic context.");
    }
    tokenTypeCheck(IDENTIFIER, className);
    
    if(className->value[0] == E_CANDY){
        *optional = true;
        
        className = consumeToken();
        tokenTypeCheck(IDENTIFIER, className);
    }
    else {
        *optional = false;
    }
    
    if(className->value[0] == E_ORANGE_TRIANGLE){
        Token *nsToken = consumeToken();
        tokenTypeCheck(IDENTIFIER, nsToken);
        *enamespace = nsToken->value[0];
        
        className = consumeToken();
        tokenTypeCheck(IDENTIFIER, className);
    }
    else {
        *enamespace = currentNamespace;
    }
    
    *typeName = className->value[0];
    
    return className;
}

/**
 * Parses and fetches a type including generic type variables.
 */
Type parseAndFetchType(Class *eclass, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType){
    if (dynamicType) {
        *dynamicType = false;
    }
    if (dynamism & AllowGenericTypeVariables && eclass && (nextToken()->type == VARIABLE || (nextToken()->value[0] == E_CANDY && nextToken()->nextToken->type == VARIABLE))) {
        if (dynamicType) {
            *dynamicType = true;
        }
        
        bool optional = false;
        Token *variableToken = consumeToken();
        if (variableToken->value[0] == E_CANDY) {
            variableToken = consumeToken();
            optional = true;
        }
        if (eclass->ownGenericArgumentVariables) {
            Type *type = dictionaryLookup(eclass->ownGenericArgumentVariables, variableToken->value, variableToken->valueLength * sizeof(EmojicodeChar));
            if (type){
                type->optional = optional;
                return *type;
            }
            else {
                String string = {variableToken->valueLength, variableToken->value};
                char *s = stringToChar(&string);
                compilerError(variableToken, "No such generic type variable \"%s\".", s);
            }
        }
        else {
            compilerError(variableToken, "Variable used as type outside generic context.");
        }
    }
    else if (nextToken()->value[0] == E_RAT) {
        Token *token = consumeToken();
        
        if(!(dynamism & AllowDynamicClassType)){
            compilerError(token, "🐀 not allowed here.");
        }
        
        if (dynamicType) {
            *dynamicType = true;
        }
        return typeForClass(eclass);
    }
    else if (nextToken()->value[0] == E_GRAPES || (nextToken()->value[0] == E_CANDY && nextToken()->nextToken->type == VARIABLE)) {
        bool optional = false;
        if (nextToken()->value[0] == E_CANDY) {
            consumeToken();
            optional = true;
        }
        consumeToken();
        
        Type t(TT_CALLABLE, optional);
        t.arguments = 0;
        
        while (!(nextToken()->type == IDENTIFIER && (nextToken()->value[0] == E_WATERMELON || nextToken()->value[0] == E_RIGHTWARDS_ARROW))) {
            t.arguments++;
            t.genericArguments[t.arguments] = parseAndFetchType(eclass, theNamespace, dynamism, NULL);
        }
        
        if(nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW){
            consumeToken();
            t.genericArguments[0] = parseAndFetchType(eclass, theNamespace, dynamism, NULL);
        }
        else {
            t.genericArguments[0] = typeNothingness;
        }
        
        Token *token = consumeToken();
        if (token->type != IDENTIFIER || token->value[0] != E_WATERMELON) {
            compilerError(token, "Expected 🍉.");
        }
        
        return t;
    }
    else {
        EmojicodeChar typeName, typeNamespace;
        bool optional, existent;
        Token *token = parseTypeName(&typeName, &typeNamespace, &optional, theNamespace);
        
        Type type = fetchRawType(typeName, typeNamespace, optional, token, &existent);
        
        if (!existent) {
            ecCharToCharStack(typeName, nameString);
            ecCharToCharStack(typeNamespace, namespaceString);
            compilerError(token, "Could not find type %s in enamespace %s.", nameString, namespaceString);
        }
        
        int offset = initializeAndCopySuperGenericArguments(&type);
        if (offset >= 0) {
            int i = 0;
            while(nextToken()->value[0] == E_SPIRAL_SHELL){
                Token *token = consumeToken();
                
                Type ta = parseAndFetchType(eclass, theNamespace, dynamism, NULL);
                validateGenericArgument(ta, i, type, token);
                type.genericArguments[offset + i] = ta;
                
                i++;
            }
            checkEnoughGenericArguments(i, type, token);
        }
        return type;
    }
}

//MARK: Type Interferring

void determineCommonType(Type t, Type *commonType, bool *firstTypeFound, Type parentType){
    if (!*firstTypeFound) {
        *commonType = t;
        *firstTypeFound = true;
    }
    else if (!typesCompatible(t, *commonType, parentType)) {
        if (typesCompatible(*commonType, t, parentType)) {
            *commonType = t;
        }
        else if(t.type == TT_CLASS && commonType->type == TT_CLASS) {
            *commonType = typeSomeobject;
        }
        else {
            *commonType = typeSomething;
        }
    }
}

void emitCommonTypeWarning(Type *commonType, bool *firstTypeFound, Token *token){
    if(!*firstTypeFound){
        *commonType = typeSomething;
        compilerWarning(token, "Type is ambigious without more context.");
    }
}

//MARK: Initializers

Type typeForClass(Class *eclass){
    Type t(TT_CLASS, false);
    t.eclass = eclass;
    
    for (int i = 0; i < eclass->genericArgumentCount; i++) {
        t.genericArguments[i] = (Type){.type = TT_REFERENCE, .optional = false, .reference = i};
    }
    
    return t;
}

Type typeForClassOptional(Class *eclass){
    Type t(TT_CLASS, optional);
    t.type = TT_CLASS;
    t.optional = true;
    t.eclass = eclass;
    
    for (int i = 0; i < eclass->genericArgumentCount; i++) {
        t.genericArguments[i] = (Type){.type = TT_REFERENCE, .optional = false, .reference = i};
    }
    
    return t;
}

bool typeIsNothingness(Type a){
    return a.type == TT_NOTHINGNESS;
}

//MARK: Type Visulisation

const char* typePackage(Type type){
    switch (type.type) {
        case TT_CLASS:
            return type.eclass->package->name;
        case TT_PROTOCOL:
            return type.protocol->package->name;
        case TT_ENUM:
            return type.eenum->package.name;
        case TT_INTEGER:
        case TT_NOTHINGNESS:
        case TT_BOOLEAN:
        case TT_SYMBOL:
        case TT_DOUBLE:
        case TT_SOMETHING:
        case TT_SOMEOBJECT:
        case TT_REFERENCE:
        case TT_CALLABLE:
            return "";
    }
}

void typeName(Type type, Type parentType, bool includeNsAndOptional, StringBuilder *sb){
    if (includeNsAndOptional) {
        if(type.optional){
            stringBuilderAppend(E_CANDY, sb);
        }
        
        switch (type.type) {
            case TT_CLASS:
                stringBuilderAppend(type.eclass->enamespace, sb);
                break;
            case TT_PROTOCOL:
                stringBuilderAppend(type.protocol->enamespace, sb);
                break;
            case TT_CALLABLE:
            case TT_REFERENCE:
                break;
            default:
                stringBuilderAppend(E_LARGE_RED_CIRCLE, sb);
                break;
        }
    }
    
    switch (type.type) {
        case TT_CLASS:
            stringBuilderAppend(type.eclass->name, sb);
            
            int offset = type.eclass->genericArgumentCount - type.eclass->ownGenericArgumentCount;
            for (int i = 0, l = type.eclass->ownGenericArgumentCount; i < l; i++) {
                stringBuilderAppend(E_SPIRAL_SHELL, sb);
                typeName(type.genericArguments[offset + i], parentType, includeNsAndOptional, sb);
            }
            
            return;
        case TT_PROTOCOL:
            stringBuilderAppend(type.protocol->name, sb);
            return;
        case TT_ENUM:
            stringBuilderAppend(type.eenum->name, sb);
            return;
        case TT_INTEGER:
            stringBuilderAppend(E_STEAM_LOCOMOTIVE, sb);
            return;
        case TT_NOTHINGNESS:
            stringBuilderAppend(E_SPARKLES, sb);
            return;
        case TT_BOOLEAN:
            stringBuilderAppend(E_OK_HAND_SIGN, sb);
            return;
        case TT_SYMBOL:
            stringBuilderAppend(E_INPUT_SYMBOL_FOR_SYMBOLS, sb);
            return;
        case TT_DOUBLE:
            stringBuilderAppend(E_ROCKET, sb);
            return;
        case TT_SOMETHING:
            stringBuilderAppend(E_MEDIUM_WHITE_CIRCLE, sb);
            return;
        case TT_SOMEOBJECT:
            stringBuilderAppend(E_LARGE_BLUE_CIRCLE, sb);
            return;
        case TT_CALLABLE:
            stringBuilderAppend(E_GRAPES, sb);
            
            for (int i = 1; i <= type.arguments; i++) {
                typeName(type.genericArguments[i], parentType, includeNsAndOptional, sb);
            }
            
            stringBuilderAppend(E_RIGHTWARDS_ARROW, sb);
            typeName(type.genericArguments[0], parentType, includeNsAndOptional, sb);
            stringBuilderAppend(E_WATERMELON, sb);
            return;
        case TT_REFERENCE: {
            if (parentType.type == TT_CLASS) {
                Class *eclass = parentType.eclass;
                do {
                    Dictionary *dict = eclass->ownGenericArgumentVariables;
                    for (size_t i = 0; i < dict->capacity; i++) {
                        if(dict->slots[i].key){
                            Type *gtype = dict->slots[i].value;
                            if (gtype->reference == type.reference) {
                                EmojicodeChar *key = dict->slots[i].key;
                                for (size_t j = 0, kl = dict->slots[i].kl / sizeof(EmojicodeChar); j < kl; j++) {
                                    stringBuilderAppend(key[j], sb);
                                }
                                return;
                            }
                        }
                    }
                } while ((eclass = eclass->superclass));
            }
            
            stringBuilderAppend('T', sb);
            stringBuilderAppend('0' + type.reference, sb);
            return;
        }
    }
}

char* typeToString(Type type, Type parentType, bool includeNsAndOptional){
    StringBuilder *sb = newStringBuilder();
    typeName(type, parentType, includeNsAndOptional, sb);
    return stringBuilderFinalize(sb);
}
