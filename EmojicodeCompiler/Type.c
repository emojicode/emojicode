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

Type typeConstraintForReference(Type t, Class *c){
    bool optional = t.optional;
    while (t.type == TT_REFERENCE) {
        t = c->genericArgumentContraints[t.reference];
    }
    t.optional = optional;
    return t;
}

Type resolveOnSuperArguments(Type t, Class *c, bool *resolved){
    while (true) {
        if (t.type != TT_REFERENCE || !c->superclass || t.reference >= c->superclass->genericArgumentCount) {
            return t;
        }
        *resolved = true;
        t = c->superGenericArguments[t.reference];
    }
}

/** Returns the name of a type */

bool typesCompatible(Type a, Type to, Type parentType){
    //(to.optional || !a.optional): Either `to` accepts optionals, or if `to` does not accept optionals `a` mustn't be one.
    if (to.type == TT_SOMETHING) {
        return true;
    }
    else if(to.type == TT_SOMEOBJECT && (a.type == TT_CLASS || a.type == TT_PROTOCOL || a.type == TT_SOMEOBJECT)){
        return to.optional || !a.optional;
    }
    else if(a.type == TT_CLASS && to.type == TT_CLASS){
        if ((to.optional || !a.optional) && inheritsFrom(a.class, to.class)) {
            if (to.class->ownGenericArgumentCount) {
                for (int l = to.class->ownGenericArgumentCount, i = to.class->genericArgumentCount - l; i < l; i++) {
                    if(!typesCompatible(a.genericArguments[i], to.genericArguments[i], parentType)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    else if(a.type == TT_PROTOCOL && a.type == TT_PROTOCOL){
        return (to.optional || !a.optional) && a.protocol == to.protocol;
    }
    else if(a.type == TT_CLASS && to.type == TT_PROTOCOL){
        return (to.optional || !a.optional) && conformsTo(a.class, to.protocol);
    }
    else if(a.type == TT_NOTHINGNESS){
        return to.optional || to.type == TT_NOTHINGNESS;
    }
    else if(a.type == TT_REFERENCE && to.type == TT_REFERENCE){
        if((to.optional || !a.optional) && a.reference == to.reference) {
            return true;
        }
        return (to.optional || !a.optional) && typesCompatible(typeConstraintForReference(a, parentType.class), typeConstraintForReference(to, parentType.class), parentType);
    }
    else if(a.type == TT_ENUM && to.type == TT_ENUM) {
        return (to.optional || !a.optional) && a.eenum == to.eenum;
    }
    else if(a.type == TT_REFERENCE) {
        bool resolved = false;
        Type rt = resolveOnSuperArguments(a, parentType.class, &resolved);
        if(resolved && (to.optional || !a.optional) && typesCompatible(rt, to, parentType)) {
            return true;
        }
        return (to.optional || !a.optional) && typesCompatible(typeConstraintForReference(a, parentType.class), to, parentType);
    }
    else if(to.type == TT_REFERENCE){
        bool resolved = false;
        Type rt = resolveOnSuperArguments(to, parentType.class, &resolved);
        if(resolved && (to.optional || !a.optional) && typesCompatible(a, rt, parentType)) {
            return true;
        }
        return (to.optional || !a.optional) && typesCompatible(a, typeConstraintForReference(to, parentType.class), parentType);
    }
    else if(a.type == TT_CALLABLE && to.type == TT_CALLABLE) {
        if (typesCompatible(a.genericArguments[0], to.genericArguments[0], parentType) && to.arguments == a.arguments) {
            for (int i = 1; i <= to.arguments; i++) {
                if (!typesCompatible(to.genericArguments[i], a.genericArguments[i], parentType)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    else {
        return (to.optional || !a.optional) && a.type == to.type;
    }
    return false;
}

Type fetchRawType(EmojicodeChar name, EmojicodeChar namespace, bool optional, Token *token, bool *existent){
    *existent = true;
    if(namespace == globalNamespace){
        switch (name) {
            case E_OK_HAND_SIGN:
                return (Type){optional, TT_BOOLEAN};
            case E_INPUT_SYMBOL_FOR_SYMBOLS:
                return (Type){optional, TT_SYMBOL};
            case E_STEAM_LOCOMOTIVE:
                return (Type){optional, TT_INTEGER};
            case E_ROCKET:
                return (Type){optional, TT_DOUBLE};
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    compilerWarning(token, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                return (Type){optional, TT_SOMETHING};
            case E_LARGE_BLUE_CIRCLE:
                return (Type){optional, TT_SOMEOBJECT};
            case E_SPARKLES:
                compilerError(token, "The Nothingness type may not be referenced to.");
        }
    }
    
    Class *class = getClass(name, namespace);
    if (class) {
        Type t = typeForClass(class);
        t.optional = optional;
        return t;
    }
    
    Protocol *protocol = getProtocol(name, namespace);
    if(protocol){
        Type t = typeForProtocol(protocol);
        t.optional = optional;
        return t;
    }
    
    Enum *eenum = getEnum(name, namespace);
    if(eenum){
        Type t = {.optional = optional, .type = TT_ENUM, .eenum = eenum};
        return t;
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
        Type *newGenericArguments = malloc(t.class->genericArgumentCount * sizeof(Type));
        for (int i = 0; i < t.class->genericArgumentCount; i++) {
            newGenericArguments[i] = resolveTypeReferences(t.genericArguments[i], o);
        }
        t.genericArguments = newGenericArguments;
    }
    return t;
}

//MARK: Type Parsing Utility

int initializeAndCopySuperGenericArguments(Type *type){
    if (type->type == TT_CLASS) {
        type->genericArguments = malloc(sizeof(Type) * type->class->genericArgumentCount);
        int offset = type->class->genericArgumentCount - type->class->ownGenericArgumentCount;
        
        if (type->class->superclass){
            memcpy(type->genericArguments, type->class->superGenericArguments, offset * sizeof(Type));
        }
        
        if (type->class->ownGenericArgumentCount){
            return offset;
        }
    }
    return -1;
}

void validateGenericArgument(Type ta, uint16_t i, Type type, Token *token){
    if(type.class->superclass){
        i += type.class->superclass->genericArgumentCount;
    }
    if(!typesCompatible(ta, type.class->genericArgumentContraints[i], type)){
        compilerError(token, "Types not matching.");
    }
}

void checkEnoughGenericArguments(uint16_t count, Type type, Token *token){
    if(count != type.class->ownGenericArgumentCount){
        char *typeString = typeToString(type, typeNothingness, true);
        compilerError(token, "Type %s requires %d generic arguments, but %d were given.", typeString, type.class->ownGenericArgumentCount, count);
    }
}

Token* parseTypeName(EmojicodeChar *typeName, EmojicodeChar *namespace, bool *optional, EmojicodeChar currentNamespace){
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
        *namespace = nsToken->value[0];
        
        className = consumeToken();
        tokenTypeCheck(IDENTIFIER, className);
    }
    else {
        *namespace = currentNamespace;
    }
    
    *typeName = className->value[0];
    
    return className;
}

/**
 * Parses and fetches a type including generic type variables.
 */
Type parseAndFetchType(Class *class, EmojicodeChar theNamespace, TypeDynamism dynamism, bool *dynamicType){
    if (dynamicType) {
        *dynamicType = false;
    }
    if (dynamism & AllowGenericTypeVariables && class && (nextToken()->type == VARIABLE || (nextToken()->value[0] == E_CANDY && nextToken()->nextToken->type == VARIABLE))) {
        if (dynamicType) {
            *dynamicType = true;
        }
        
        bool optional = false;
        Token *variableToken = consumeToken();
        if (variableToken->value[0] == E_CANDY) {
            variableToken = consumeToken();
            optional = true;
        }
        if (class->ownGenericArgumentVariables) {
            Type *type = dictionaryLookup(class->ownGenericArgumentVariables, variableToken->value, variableToken->valueLength * sizeof(EmojicodeChar));
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
        return typeForClass(class);
    }
    else if (nextToken()->value[0] == E_GRAPES || (nextToken()->value[0] == E_CANDY && nextToken()->nextToken->type == VARIABLE)) {
        bool optional = false;
        if (nextToken()->value[0] == E_CANDY) {
            consumeToken();
            optional = true;
        }
        consumeToken();
        
        Type t;
        t.type = TT_CALLABLE;
        t.arguments = 0;
        t.optional = optional;
        
        int size = 5;
        t.genericArguments = malloc(sizeof(Type) * size);
        
        while (!(nextToken()->type == IDENTIFIER && (nextToken()->value[0] == E_WATERMELON || nextToken()->value[0] == E_RIGHTWARDS_ARROW))) {
            t.arguments++;
            if(t.arguments == size){
                size *= 2;
                t.genericArguments = realloc(t.genericArguments, size * sizeof(Type));
            }
            t.genericArguments[t.arguments] = parseAndFetchType(class, theNamespace, dynamism, NULL);
        }
        
        if(nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW){
            consumeToken();
            t.genericArguments[0] = parseAndFetchType(class, theNamespace, dynamism, NULL);
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
            compilerError(token, "Could not find type %s in namespace %s.", nameString, namespaceString);
        }
        
        int offset = initializeAndCopySuperGenericArguments(&type);
        if (offset >= 0) {
            int i = 0;
            while(nextToken()->value[0] == E_SPIRAL_SHELL){
                Token *token = consumeToken();
                
                Type ta = parseAndFetchType(class, theNamespace, dynamism, NULL);
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

Type typeForClass(Class *class){
    Type t;
    t.type = TT_CLASS;
    t.optional = false;
    t.class = class;
    t.genericArguments = malloc(class->genericArgumentCount * sizeof(Type));
    
    for (int i = 0; i < class->genericArgumentCount; i++) {
        t.genericArguments[i] = (Type){.type = TT_REFERENCE, .optional = false, .reference = i};
    }
    
    return t;
}

Type typeForClassOptional(Class *class){
    Type t;
    t.type = TT_CLASS;
    t.optional = true;
    t.class = class;
    t.genericArguments = malloc(class->genericArgumentCount * sizeof(Type));
    
    for (int i = 0; i < class->genericArgumentCount; i++) {
        t.genericArguments[i] = (Type){.type = TT_REFERENCE, .optional = false, .reference = i};
    }
    
    return t;
}

Type typeForProtocol(Protocol *protocol){
    Type t = {.type = TT_PROTOCOL, .optional = false, .protocol = protocol};
    return t;
}

Type typeForProtocolOptional(Protocol *protocol){
    Type t = {.type = TT_PROTOCOL, .optional = true, .protocol = protocol};
    return t;
}

bool typeIsClass(Type type){
    return true;
}

bool typeIsNothingness(Type a){
    return a.type == TT_NOTHINGNESS;
}

//MARK: Type Visulisation

const char* typePackage(Type type){
    switch (type.type) {
        case TT_CLASS:
            return type.class->package->name;
        case TT_PROTOCOL:
            return type.protocol->package->name;
        case TT_ENUM:
            return type.eenum->package->name;
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

typedef struct {
    size_t size;
    char *string;
    size_t index;
} StringBuilder;

void stringBuilderAppend(EmojicodeChar c, StringBuilder *builder){
    size_t l = u8_charlen(c);
    if (builder->index + l >= builder->size) {
        builder->string = realloc(builder->string, builder->size *= 2);
    }
    
    u8_wc_toutf8(builder->string + builder->index, c);
    builder->index += l;
}

char* stringBuilderFinalize(StringBuilder *builder){
    builder->string[builder->index] = '\0';
    char *str = builder->string;
    free(builder);
    return str;
}

StringBuilder* newStringBuilder(){
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    sb->size = 12;
    sb->index = 0;
    sb->string = malloc(sb->size);
    return sb;
}

void typeName(Type type, Type parentType, bool includeNsAndOptional, StringBuilder *sb){
    if (includeNsAndOptional) {
        if(type.optional){
            stringBuilderAppend(E_CANDY, sb);
        }
        
        switch (type.type) {
            case TT_CLASS:
                stringBuilderAppend(type.class->namespace, sb);
                break;
            case TT_PROTOCOL:
                stringBuilderAppend(type.protocol->namespace, sb);
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
            stringBuilderAppend(type.class->name, sb);
            
            int offset = type.class->genericArgumentCount - type.class->ownGenericArgumentCount;
            for (int i = 0, l = type.class->ownGenericArgumentCount; i < l; i++) {
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
                Class *class = parentType.class;
                do {
                    Dictionary *dict = class->ownGenericArgumentVariables;
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
                } while ((class = class->superclass));
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
