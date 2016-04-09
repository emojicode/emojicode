//
//  Procedure.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <map>
#include "utf8.h"
#include "Procedure.hpp"
#include "Lexer.hpp"
#include "EmojicodeCompiler.hpp"

void Callable::parseArgumentList(TypeContext ct, Package *package) {
    const Token *token;
    
    while ((token = nextToken()) && token->type == VARIABLE) {
        auto variableToken = consumeToken(VARIABLE);
        auto type = Type::parseAndFetchType(ct, AllowGenericTypeVariables, package);
        
        arguments.push_back(Variable(Variable(variableToken, type)));
    }
    
    if (arguments.size() > UINT8_MAX) {
        compilerError(token, "A function cannot take more than 255 arguments.");
    }
}

void Callable::parseReturnType(TypeContext ct, Package *package) {
    if (nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_RIGHTWARDS_ARROW) {
        consumeToken();
        returnType = Type::parseAndFetchType(ct, AllowGenericTypeVariables, package);
    }
}

//MARK: Closure

Type Closure::type() {
    Type t(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (int i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

//MARK: Procedure

static const Token* until(EmojicodeChar end, EmojicodeChar deeper, int *deep) {
    const Token *token = consumeToken();
    
    if (token == nullptr) {
        compilerError(nullptr, "Unexpected end of program.");
    }
    
    if (token->type != IDENTIFIER) {
        return token;
    }
    
    if (token->value[0] == deeper) {
        (*deep)++;
    }
    else if (token->value[0] == end) {
        if (*deep == 0) {
            return nullptr;
        }
        (*deep)--;
    }
    
    return token;
}

void Procedure::parseGenericArguments(TypeContext ct, Package *package) {
    const Token *token;
    
    //Until the grape is found we parse arguments
    while ((token = nextToken()) && token->type == IDENTIFIER && token->value[0] == E_SPIRAL_SHELL) {
        consumeToken();
        const Token *variable = consumeToken(VARIABLE);
        
        Type t = Type::parseAndFetchType(Type(eclass), AllowGenericTypeVariables, package, nullptr);
        genericArgumentConstraints.push_back(t);
        
        Type rType(TT_LOCAL_REFERENCE, false);
        rType.reference = genericArgumentVariables.size();
        
        if (genericArgumentVariables.count(variable->value)) {
            compilerError(variable, "A generic argument variable with the same name is already in use.");
        }
        genericArgumentVariables.insert(std::map<EmojicodeString, Type>::value_type(variable->value, rType));
    }
}

void Procedure::parseBody(bool allowNative) {
    if (nextToken()->value[0] == E_RADIO) {
        const Token *t = consumeToken(IDENTIFIER);
        if (!allowNative) {
            compilerError(t, "Native code is not allowed in this context.");
        }
        native = true;
        return;
    }
    
    const Token *token = consumeToken(IDENTIFIER);
    
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], c);
        compilerError(token, "Expected 🍇 but found %s instead.", c);
    }
    
    firstToken = currentToken;
    
    int d = 0;
    while ((token = until(E_WATERMELON, E_GRAPES, &d)) != nullptr) continue;
}

void Procedure::checkPromises(Procedure *superProcedure, const char *on, Type contextType) {
    if (superProcedure->final) {
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s of %s was marked 🔏.", on, mn);
    }
    if (!this->returnType.compatibleTo(superProcedure->returnType, contextType)) {
        // The return type may be defined more precisely
        ecCharToCharStack(this->name, mn);
        auto supername = superProcedure->returnType.toString(contextType, true);
        auto thisname = this->returnType.toString(contextType, true);
        compilerError(this->dToken, "Return type %s of %s is not compatible with the return type %s of its %s.",
                      thisname.c_str(), mn, supername.c_str(), on);
    }
    if (superProcedure->arguments.size() != this->arguments.size()) {
        ecCharToCharStack(this->name, mn);
        compilerError(this->dToken, "%s expects %s arguments than its %s.", mn,
                      (superProcedure->arguments.size() < this->arguments.size()) ? "more" : "less", on);
    }
    for (uint8_t i = 0; i < superProcedure->arguments.size(); i++) {
        // Other way, because the procedure may take more general arguments
        if (!superProcedure->arguments[i].type.compatibleTo(this->arguments[i].type, contextType)) {
            auto supertype = superProcedure->arguments[i].type.toString(contextType, true);
            auto thisname = this->arguments[i].type.toString(contextType, true);
            compilerError(superProcedure->dToken,
                          "Type %s of argument %d is not compatible with its %s argument type %s.",
                          thisname.c_str(), i + 1, on, supertype.c_str());
        }
    }
}

void Procedure::checkOverride(Procedure *superProcedure) {
    if (overriding && !superProcedure) {
        ecCharToCharStack(name, mn);
        compilerError(dToken, "%s was declared ✒️ but does not override anything.", mn);
    }
    else if (!overriding && superProcedure) {
        ecCharToCharStack(name, mn);
        compilerError(dToken, "If you want to override %s add ✒️.", mn);
    }
}

void Procedure::deprecatedWarning(const Token *callToken) {
    if (deprecated) {
        ecCharToCharStack(name, mn);
        if (documentationToken) {
            char *documentation = documentationToken->value.utf8CString();
            compilerWarning(callToken,
                            "%s is deprecated. Please refer to the documentation for further information:\n%s",
                            mn, documentation);
            delete [] documentation;
        }
        else {
            compilerWarning(callToken, "%s is deprecated.", mn);
        }
    }
}

Type Procedure::type() {
    Type t = Type(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(returnType);
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}

Type Initializer::type() {
    Type t = Type(TT_CALLABLE, false);
    t.arguments = (uint8_t)arguments.size();
    
    t.genericArguments.push_back(Type(eclass, canReturnNothingness));
    for (size_t i = 0; i < arguments.size(); i++) {
        t.genericArguments.push_back(arguments[i].type);
    }
    return t;
}
