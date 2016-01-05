//
//  StaticAnalyzer.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <string.h>
#include <limits.h>
#include "utf8.h"
#include "Lexer.h"
#include "StaticAnalyzer.h"
#include "EmojicodeCompiler.h"
#include "Writer.h"
#include "ClassParser.h"
#include "CompilerScope.h"

static FILE *out;
static std::vector<Token *> stringPool;

#define noEffectWarning(token) compilerWarning(token, "Statement seems to have no effect whatsoever.");

//MARK: Type Checking and Safety

Type safeParse(Token *token, Token *parentToken, StaticInformation *SI){
    if(token == NULL || token->type == NO_TYPE){
        compilerError(token, "Unexpected end of function body.");
    }
    return typeParse(token, SI);
}

Type safeParseTypeConstraint(Token *token, Token *parentToken, Type type, StaticInformation *SI){
    if(token == NULL || token->type == NO_TYPE){
        compilerError(parentToken, "Unexpected end of function body.");
    }
    Type v = typeParse(token, SI);
    if(!v.compatibleTo(type, SI->classTypeContext)){
        auto cn = v.toString(SI->classTypeContext, true);
        auto tn = type.toString(SI->classTypeContext, true);
        compilerError(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return v;
}


//MARK: Block utilities

/** Handles a complete block */
static void block(StaticInformation *SI){
    currentScopeWrapper->scope->changeInitializedBy(1);
    if (!SI->inClassContext) {
        currentScopeWrapper->topScope->scope->changeInitializedBy(1);
    }
    
    SI->flowControlDepth++;
    
    Token *token = consumeToken();
    token->forceType(IDENTIFIER);
    if (token->value[0] != E_GRAPES){
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    
    off_t pp = writePlaceholderCoin(out);
        
    uint32_t delta = writtenCoins;
    
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        SI->effect = false;
        typeParse(token, SI);
        if(!SI->effect){
            noEffectWarning(token);
        }
    }
    
    writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
    
    SI->effect = true;
}

static void blockDepthDecrement(StaticInformation *SI){
    currentScopeWrapper->scope->changeInitializedBy(-1);
    if (!SI->inClassContext) {
        currentScopeWrapper->topScope->scope->changeInitializedBy(-1);
    }
    
    SI->flowControlDepth--;
    SI->returned = false;
}

//MARK: Static Information

#define dynamismLevelFromSI(SI) ((SI)->inClassContext ? AllowDynamicClassType : NoDynamism)

static uint8_t nextVariableID(StaticInformation *SI){
    return SI->variableCount++;
}

static void noReturnError(Token *errorToken, StaticInformation *SI){
    if (SI->returnType.type != TT_NOTHINGNESS && !SI->returned) {
        compilerError(errorToken, "An explicit return is missing.");
    }
}

//MARK: Low level parsing

void checkArguments(Arguments arguments, Type calledType, Token *token, StaticInformation *SI){
    for (auto var : arguments) {
        safeParseTypeConstraint(consumeToken(), token, var.type.resolveOn(calledType), SI);
    }
}

static void checkAccess(Procedure *p, Token *token, const char *type, StaticInformation *SI){
    if (p->access == PRIVATE) {
        if (p->eclass != SI->classTypeContext.eclass) {
            ecCharToCharStack(p->name, nm);
            compilerError(token, "%s %s is 🔒.", type, nm);
        }
    }
    else if(p->access == PROTECTED) {
        if (!SI->classTypeContext.eclass->inheritsFrom(p->eclass)) {
            ecCharToCharStack(p->name, nm);
            compilerError(token, "%s %s is 🔐.", type, nm);
        }
    }
}

void writeCoinForScopesUp(uint8_t scopesUp, Token *varName, EmojicodeCoin stack, EmojicodeCoin object, StaticInformation *SI){
    if(scopesUp == 0){
        writeCoin(stack, out);
    }
    else if(scopesUp == 1){
        writeCoin(object, out);
        SI->usedSelf = true;
    }
    else {
        compilerError(varName, "The variable cannot be resolved correctly.");
    }
}

static void parseIfExpression(Token *token, StaticInformation *SI){
    if (nextToken()->value[0] == E_SOFT_ICE_CREAM) {
        consumeToken();
        writeCoin(0x3E, out);
        
        Token *varName = consumeToken();
        varName->forceType(VARIABLE);
        
        if(currentScopeWrapper->scope->getLocalVariable(varName) != NULL){
            compilerError(token, "Cannot redeclare variable.");
        }
        
        uint8_t id = nextVariableID(SI);
        writeCoin(id, out);
        
        Type t = safeParse(consumeToken(), token, SI);
        if (!t.optional) {
            compilerError(token, "🍊🍦 can only be used with optionals.");
        }
        
        t.optional = false;
        currentScopeWrapper->scope->setLocalVariable(varName, new CompilerVariable(t, id, 1, true));
    }
    else {
        safeParseTypeConstraint(consumeToken(), token, typeBoolean, SI);
    }
}

Type typeParseIdentifier(Token *token, StaticInformation *SI){
    if(token->value[0] != E_RED_APPLE){
        //We need a chance to test whether the red apple’s return is used
        SI->effect = true;
    }
        
    switch (token->value[0]) {
        case E_SHORTCAKE: {
            Token *varName = consumeToken();
            varName->forceType(VARIABLE);
            
            if (currentScopeWrapper->scope->getLocalVariable(varName) != NULL) {
                compilerError(token, "Cannot redeclare variable.");
            }
            
            Type t = parseAndFetchType(SI->classTypeContext.eclass, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            uint8_t id = nextVariableID(SI);
            currentScopeWrapper->scope->setLocalVariable(varName, new CompilerVariable(t, id, t.optional ? 1 : 0, false));
            
            return typeNothingness;
        }
        case E_CUSTARD: {
            Token *varName = consumeToken();
            varName->forceType(VARIABLE);
            
            uint8_t scopesUp;
            CompilerVariable *cv = getVariable(varName, &scopesUp);
            if(cv == NULL){
                //Not declared, declaring as local variable

                writeCoin(0x1B, out);
                
                uint8_t id = nextVariableID(SI);
                
                writeCoin(id, out);
                
                Type t = safeParse(consumeToken(), token, SI);
                currentScopeWrapper->scope->setLocalVariable(varName, new CompilerVariable(t, id, 1, false));
            }
            else {
                if (cv->initialized <= 0) {
                    cv->initialized = 1;
                }
                
                cv->frozenError(varName);
                
                writeCoinForScopesUp(scopesUp, varName, 0x1B, 0x1D, SI);
                writeCoin(cv->id, out);
                
                safeParseTypeConstraint(consumeToken(), token, cv->type, SI);
            }
            return typeNothingness;
        }
        case E_SOFT_ICE_CREAM: {
            Token *varName = consumeToken();
            varName->forceType(VARIABLE);
            
            if(currentScopeWrapper->scope->getLocalVariable(varName) != NULL){
                compilerError(token, "Cannot redeclare variable.");
            }
            
            
            writeCoin(0x1B, out);
            
            uint8_t id = nextVariableID(SI);
            writeCoin(id, out);
            
            Type t = safeParse(consumeToken(), token, SI);
            currentScopeWrapper->scope->setLocalVariable(varName, new CompilerVariable(t, id, 1, true));
            return typeNothingness;
        }
        case E_COOKING:
        case E_CHOCOLATE_BAR: {
            Token *varName = consumeToken();
            varName->forceType(VARIABLE);
            
            //Fetch the old value
            uint8_t scopesUp;
            CompilerVariable *cv = getVariable(varName, &scopesUp);
            
            if (!cv) {
                compilerError(token, "Unknown variable \"%s\"", varName->value.utf8CString());
                break;
            }
            
            cv->uninitalizedError(varName);
            cv->frozenError(varName);
            
            if (!cv->type.compatibleTo(typeInteger, SI->classTypeContext)) {
                ecCharToCharStack(token->value[0], ls);
                compilerError(token, "%s can only operate on 🚂 variables.", ls);
            }
            
            if (token->value[0] == E_COOKING) { //decrement
                writeCoinForScopesUp(scopesUp, varName, 0x19, 0x1F, SI);
            }
            else {
                writeCoinForScopesUp(scopesUp, varName, 0x18, 0x1E, SI);
            }
            
            writeCoin(cv->id, out);
            
            return typeNothingness;
        }
        case E_COOKIE: {
            writeCoin(0x52, out);
            off_t pp = writePlaceholderCoin(out);
            
            uint32_t n = 1;
            
            safeParseTypeConstraint(consumeToken(), token, Type(CL_STRING), SI);
            
            Token *stringToken;
            while (stringToken = consumeToken(), !(stringToken->type == IDENTIFIER && stringToken->value[0] == E_COOKIE)) {
                safeParseTypeConstraint(stringToken, token, Type(CL_STRING), SI);
                n++;
            }
            
            writeCoinAtPlaceholder(pp, n, out);
            return Type(CL_STRING);
        }
        case E_ICE_CREAM: {
            writeCoin(0x51, out);
            off_t pp = writePlaceholderCoin(out);
            
            uint32_t delta = writtenCoins;
            
            CommonTypeFinder ct;
            
            Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                ct.addType(safeParse(aToken, token, SI), SI->classTypeContext);
            }
            
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            
            Type type = Type(CL_LIST);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_HONEY_POT: {
            writeCoin(0x50, out);
            off_t pp = writePlaceholderCoin(out);
            
            uint32_t delta = writtenCoins;
            
            CommonTypeFinder ct;
            
            Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                safeParseTypeConstraint(aToken, token, Type(CL_STRING), SI);
                ct.addType(safeParse(consumeToken(), token, SI), SI->classTypeContext);
            }
            
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            
            Type type = Type(CL_DICTIONARY);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_TANGERINE: { //MARK: If
            writeCoin(0x62, out);
            
            off_t pp = writePlaceholderCoin(out);
            uint32_t delta = writtenCoins;
            
            parseIfExpression(token, SI);
            
            block(SI);
            blockDepthDecrement(SI);

            while ((token = nextToken()) != NULL && token->type == IDENTIFIER && token->value[0] == E_LEMON) {
                writeCoin(consumeToken()->value[0], out);
                
                parseIfExpression(token, SI);

                block(SI);
                blockDepthDecrement(SI);
            }
            
            if((token = nextToken()) != NULL && token->type == IDENTIFIER && token->value[0] == E_STRAWBERRY){
                writeCoin(consumeToken()->value[0], out);
                block(SI);
                blockDepthDecrement(SI);
            }
            
            writeCoinAtPlaceholder(pp, writtenCoins - delta + 1, out);
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
            writeCoin(0x61, out);
            
            safeParseTypeConstraint(consumeToken(), token, typeBoolean, SI);
            
            block(SI);
            blockDepthDecrement(SI);
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            off_t pp = writePlaceholderCoin(out);
            
            //The destination variable
            Token *variableToken = consumeToken();
            variableToken->forceType(VARIABLE);
            
            if (currentScopeWrapper->scope->getLocalVariable(variableToken) != NULL) {
                compilerError(variableToken, "Cannot redeclare variable.");
            }
            
            uint8_t vID = nextVariableID(SI);
            writeCoin(vID, out);
            //Internally needed
            writeCoin(nextVariableID(SI), out);
            
            Type iteratee = safeParseTypeConstraint(consumeToken(), token, typeSomeobject, SI);
            
            if(iteratee.type == TT_CLASS && iteratee.eclass == CL_LIST) {
                //If the iteratee is a list, the Real-Time Engine has some special sugar
                writeCoinAtPlaceholder(pp, 0x65, out);
                currentScopeWrapper->scope->setLocalVariable(variableToken, new CompilerVariable(iteratee.genericArguments[0], vID, true, false));
            }
            else if(iteratee.compatibleTo(Type(PR_ENUMERATEABLE, false), SI->classTypeContext)) {
                writeCoinAtPlaceholder(pp, 0x64, out);
                Type itemType = typeSomething;
                if(iteratee.type == TT_CLASS && iteratee.eclass->ownGenericArgumentCount == 1) {
                    itemType = iteratee.genericArguments[iteratee.eclass->ownGenericArgumentCount - iteratee.eclass->genericArgumentCount];
                }
                currentScopeWrapper->scope->setLocalVariable(variableToken, new CompilerVariable(itemType, vID, true, false));
            }
            else {
                auto iterateeString = SI->classTypeContext.toString(iteratee, true);
                compilerError(token, "%s does not conform to 🔴🔂.", iterateeString.c_str());
            }
            
            block(SI);
            blockDepthDecrement(SI);
            
            return typeNothingness;
        }
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_RAT: {
            ecCharToCharStack(token->value[0], identifier);
            compilerError(token, "Unexpected identifier %s.", identifier);
            return typeNothingness;
        }
        case E_DOG: {
            SI->usedSelf = true;
            writeCoin(0x3C, out);
            if (SI->initializer && !SI->calledSuper && SI->initializer->eclass->superclass) {
                compilerError(token, "Attempt to use 🐕 before superinitializer call.");
            }
            
            if (SI->inClassContext) {
                compilerError(token, "Illegal use of 🐕.", token->value[0]);
                break;
            }
            
            currentScopeWrapper->topScope->scope->initializerUnintializedVariablesCheck(token, "Instance variable \"%s\" must be initialized before the use of 🐕.");

            return SI->classTypeContext;
        }
        case E_UP_POINTING_RED_TRIANGLE: {
            writeCoin(0x13, out);
            
            Type type = parseAndFetchType(SI->classTypeContext.eclass, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            if (type.type != TT_ENUM) {
                compilerError(token, "The given type cannot be accessed.");
            }
            else if (type.optional) {
                compilerError(token, "Optionals cannot be accessed.");
            }
            
            Token *name = consumeToken();
            name->forceType(IDENTIFIER);
            
            auto v = type.eenum->getValueFor(name->value[0]);
            if (!v.first) {
                ecCharToCharStack(name->value[0], valueName);
                ecCharToCharStack(type.eenum->name, enumName);
                compilerError(name, "%s does not have a member named %s.", enumName, valueName);
            }
            else if (v.second > UINT32_MAX) {
                writeCoin((v.second >> 32), out);
                writeCoin((EmojicodeCoin)v.second, out);
            }
            else {
                writeCoin((EmojicodeCoin)v.second, out);
            }
            
            return type;
        }
        case E_LARGE_BLUE_DIAMOND: {
            writeCoin(0x4, out);
            
            bool dynamic;
            Type type = parseAndFetchType(SI->classTypeContext.eclass, SI->currentNamespace, dynamismLevelFromSI(SI), &dynamic);
            
            if (type.type != TT_CLASS) {
                compilerError(token, "The given type cannot be initiatied.");
            }
            else if (type.optional) {
                compilerError(token, "Optionals cannot be initiatied.");
            }
            
            if (dynamic) {
                writeCoin(UINT32_MAX, out);
            }
            else {
                writeCoin(type.eclass->index, out);
            }
            
            //The initializer name
            Token *consName = consumeToken();
            consName->forceType(IDENTIFIER);
            
            Initializer *initializer = type.eclass->getInitializer(consName->value[0]);
            
            if (initializer == NULL) {
                auto typeString = type.toString(SI->classTypeContext, true);
                ecCharToCharStack(consName->value[0], initializerString);
                compilerError(consName, "%s has no initializer %s.", typeString.c_str(), initializerString);
            }
            else if (dynamic && !initializer->required) {
                compilerError(consName, "Only required initializers can be used with 🐀.");
            }
            
            writeCoin(initializer->vti, out);
            
            checkAccess(initializer, token, "Initializer", SI);
            checkArguments(initializer->arguments, type, token, SI);
            
            if (initializer->canReturnNothingness) {
                type.optional = true;
            }
            return type;
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writeCoin(0x17, out);
            return typeNothingness;
        }
        case E_CLOUD: {
            writeCoin(0x2E, out);
            safeParse(consumeToken(), token, SI);
            return typeBoolean;
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writeCoin(0x2D, out);
            
            safeParseTypeConstraint(consumeToken(), token, typeSomeobject, SI);
            safeParseTypeConstraint(consumeToken(), token, typeSomeobject, SI);
            
            return typeBoolean;
        }
        case E_GOAT: {
            if (!SI->initializer) {
                compilerError(token, "🐐 can only be used inside initializers.");
                break;
            }
            if (!SI->classTypeContext.eclass->superclass) {
                compilerError(token, "🐐 can only be used if the eclass inherits from another.");
                break;
            }
            if (SI->calledSuper) {
                compilerError(token, "You may not call more than one superinitializer.");
            }
            if (SI->flowControlDepth) {
                compilerError(token, "You may not put a call to a superinitializer in a flow control structure.");
            }
            
            currentScopeWrapper->topScope->scope->initializerUnintializedVariablesCheck(token, "Instance variable \"%s\" must be initialized before superinitializer.");
            
            writeCoin(0x3D, out);
            
            Class *eclass = SI->classTypeContext.eclass;
            
            writeCoin(eclass->superclass->index, out);
            
            Token *initializerToken = consumeToken();
            initializerToken->forceType(IDENTIFIER);
            
            Initializer *initializer = eclass->superclass->getInitializer(initializerToken->value[0]);
            
            if (initializer == NULL) {
                ecCharToCharStack(initializerToken->value[0], initializerString);
                compilerError(initializerToken, "Cannot find superinitializer %s.", initializerString);
                break;
            }
            
            writeCoin(initializer->vti, out);
            
            checkAccess(initializer, token, "initializer", SI);
            checkArguments(initializer->arguments, SI->classTypeContext, token, SI);

            SI->calledSuper = true;
            
            return typeNothingness;
        }
        case E_RED_APPLE: {
            if(SI->effect){
                //effect is true, so apple is called as subcommand
                compilerError(token, "🍎’s return may not be used as an argument.");
            }
            SI->effect = true;
            
            writeCoin(0x60, out);
            
            if(SI->initializer){
                if (SI->initializer->canReturnNothingness) {
                    safeParseTypeConstraint(consumeToken(), token, typeNothingness, SI);
                    return typeNothingness;
                }
                else {
                    compilerError(token, "🍎 cannot be used inside a initializer.");
                }
            }
            
            safeParseTypeConstraint(consumeToken(), token, SI->returnType, SI);
            SI->returned = true;
            return typeNothingness;
        }
        case E_BLACK_SQUARE_BUTTON: {
            off_t pp = writePlaceholderCoin(out);
            
            Type originalType = safeParseTypeConstraint(consumeToken(), token, typeSomething, SI);
            bool dynamic;
            Type type = parseAndFetchType(SI->classTypeContext.eclass, SI->currentNamespace, dynamismLevelFromSI(SI), &dynamic);
            
            if (dynamic) {
                compilerError(token, "You cannot cast to dynamic types.");
            }
            
            if (originalType.compatibleTo(type, SI->classTypeContext)) {
                compilerWarning(token, "Superfluous cast.");
            }
            
            switch (type.type) {
                case TT_CLASS:
                    for (size_t i = 0; i < type.eclass->ownGenericArgumentCount; i++) {
                        if(!type.eclass->genericArgumentContraints[i].compatibleTo(type.genericArguments[i], type) ||
                           !type.genericArguments[i].compatibleTo(type.eclass->genericArgumentContraints[i], type)) {
                            compilerError(token, "Dynamic casts involving generic type arguments are not possible yet. Please specify the generic argument constraints of the class for compatibility with future versions.");
                        }
                    }

                    writeCoinAtPlaceholder(pp, originalType.type == TT_SOMETHING || originalType.optional ? 0x44 : 0x40, out);
                    writeCoin(type.eclass->index, out);
                    break;
                case TT_PROTOCOL:
                    writeCoinAtPlaceholder(pp, originalType.type == TT_SOMETHING || originalType.optional ? 0x45 : 0x41, out);
                    writeCoin(type.protocol->index, out);
                    break;
                case TT_BOOLEAN:
                    writeCoinAtPlaceholder(pp, 0x42, out);
                    break;
                case TT_INTEGER:
                    writeCoinAtPlaceholder(pp, 0x43, out);
                    break;
                case TT_SYMBOL:
                    writeCoinAtPlaceholder(pp, 0x46, out);
                    break;
                case TT_DOUBLE:
                    writeCoinAtPlaceholder(pp, 0x47, out);
                    break;
                default: {
                    auto typeString = type.toString(SI->classTypeContext, true);
                    compilerError(token, "You cannot cast to %s.", typeString.c_str());
                }
            }
            
            type.optional = true;
            return type;
        }
        case E_BEER_MUG: {
            writeCoin(0x3A, out);
            
            Type t = safeParse(consumeToken(), token, SI);
            
            if (!t.optional) {
                compilerError(token, "🍺 can only be used with optionals.");
            }
            
            t.optional = false;
            
            return t;
        }
        case E_CLINKING_BEER_MUGS: {
            writeCoin(0x3B, out);
            
            off_t pp = writePlaceholderCoin(out);
            uint32_t delta = writtenCoins;
            
            Token *methodToken = consumeToken();
            
            Type type = safeParse(consumeToken(), token, SI);
            if(!type.optional){
                compilerError(token, "🍻 may only be used on 🍬.");
            }
            
            Method *method = type.eclass->getMethod(methodToken->value[0]);
            
            if(method == NULL){
                auto eclass = type.toString(SI->classTypeContext, true);
                ecCharToCharStack(methodToken->value[0], method);
                compilerError(token, "%s has no method %s", eclass.c_str(), method);
            }
            
            writeCoin(method->vti, out);
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            
            checkAccess(method, token, "method", SI);
            checkArguments(method->arguments, type, token, SI);
            
            Type returnType = method->returnType;
            returnType.optional = true;
            return returnType;
        }
        case E_DOUGHNUT: {
            writeCoin(0x2, out);
            
            Token *methodToken = consumeToken();
            methodToken->forceType(IDENTIFIER);
            
            Type type = parseAndFetchType(SI->classTypeContext.eclass, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            if (type.optional) {
                compilerWarning(token, "Please remove useless 🍬.");
            }
            if (type.type != TT_CLASS) {
                compilerError(token, "The given type is not a eclass.");
            }
            
            writeCoin(type.eclass->index, out);
            
            ClassMethod *method = type.eclass->getClassMethod(methodToken->value[0]);
            
            if (method == NULL) {
                auto classString = type.toString(SI->classTypeContext, true);
                ecCharToCharStack(methodToken->value[0], methodString);
                compilerError(token, "%s has no eclass method %s", classString.c_str(), methodString);
            }
            
            writeCoin(method->vti, out);
            
            checkAccess(method, token, "Class method", SI);
            checkArguments(method->arguments, type, token, SI);
            
            return method->returnType.resolveOn(type);
        }
        case E_HOT_PEPPER: {
            Token *methodName = consumeToken();
            
            writeCoin(0x71, out);
            
            Type type = safeParse(consumeToken(), token, SI);
            
            Method *method;
            if (type.type != TT_CLASS) {
                compilerError(token, "You can only capture method calls on class instances.");
            }
            method = type.eclass->getMethod(methodName->value[0]);
            
            if (!method) {
                compilerError(token, "Method is non-existent.");
            }
            
            writeCoin(method->vti, out);
            
            Type t(TT_CALLABLE, false);
            t.type = TT_CALLABLE;
            t.arguments = (uint8_t)method->arguments.size();
            
            t.genericArguments.push_back(method->returnType);
            for (size_t i = 0; i < method->arguments.size(); i++) {
                t.genericArguments.push_back(method->arguments[i].type);
            }
            return t;
        }
        case E_GRAPES: {
            writeCoin(0x70, out);
            
            Type t(TT_CALLABLE, false);
            
            auto arguments = parseArgumentList(SI->classTypeContext.eclass, SI->currentNamespace);
            
            t.arguments = (uint8_t)arguments.size();
            
            t.genericArguments.push_back(parseReturnType(SI->classTypeContext.eclass, SI->currentNamespace));
            for (int i = 0; i < arguments.size(); i++) {
                t.genericArguments.push_back(arguments[i].type);
            }
            
            off_t variableCountPp = writePlaceholderCoin(out);
            off_t pp = writePlaceholderCoin(out);
            uint32_t delta = writtenCoins;
            
            uint8_t preVarID = SI->variableCount;
            Type preReturn = SI->returnType;
            int preFlowControlDepth = SI->flowControlDepth;
            bool preEffect = SI->effect;
            bool preReturned = SI->returned;
            
            SI->usedSelf = false;
            SI->returnType = t.genericArguments[0];
            
            Scope *closingScope = currentScopeWrapper->scope;
            if (!SI->inClassContext) {
                pushScope(currentScopeWrapper->topScope->scope);
            }
            
            analyzeFunctionBodyFull(currentToken, arguments, SI, true, closingScope);
            noReturnError(token, SI);
            
            if (!SI->inClassContext) {
                popScope();
            }
            
            writeCoinAtPlaceholder(variableCountPp, SI->variableCount, out);
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            writeCoin((uint32_t)arguments.size() | (SI->usedSelf ? 1 << 16 : 0), out);
            writeCoin(preVarID, out);
            
            SI->variableCount = preVarID;
            SI->flowControlDepth = preFlowControlDepth;
            SI->effect = preEffect;
            SI->returnType = preReturn;
            SI->returned = preReturned;
            
            return t;
        }
        case E_LOLLIPOP: {
            writeCoin(0x72, out);
            
            Type type = safeParse(consumeToken(), token, SI);
            
            if (type.type != TT_CALLABLE) {
                compilerError(token, "Given value is not callable.");
            }
            
            for (int i = 1; i <= type.arguments; i++) {
                safeParseTypeConstraint(consumeToken(), token, type.genericArguments[i], SI);
            }
            
            return type.genericArguments[0];
        }
        case E_CHIPMUNK: {
            Token *nameToken = consumeToken();
            
            Class *superclass = SI->classTypeContext.eclass->superclass;
            Method *method = superclass->getMethod(nameToken->value[0]);
            
            if (!method) {
                compilerError(token, "Method is non-existent.");
            }
            
            writeCoin(0x5, out);
            writeCoin(superclass->index, out);
            writeCoin(method->vti, out);
            
            checkArguments(method->arguments, SI->classTypeContext, token, SI);
            
            return method->returnType;
        }
        default:
        {
            off_t pp = writePlaceholderCoin(out);
            
            Token *tobject = consumeToken();
            
            Type type = safeParse(tobject, token, SI);
            
            if (type.optional) {
                compilerError(tobject, "You cannot call methods on optionals.");
            }
            
            Method *method;
            if(type.type == TT_PROTOCOL){
                method = type.protocol->getMethod(token->value[0]);
            }
            else if(type.type == TT_CLASS) {
                method = type.eclass->getMethod(token->value[0]);
            }
            else {
                if(type.type == TT_BOOLEAN){
                    switch (token->value[0]) {
                        case E_NEGATIVE_SQUARED_CROSS_MARK:
                            writeCoinAtPlaceholder(pp, 0x26, out);
                            return typeBoolean;
                        case E_PARTY_POPPER:
                            writeCoinAtPlaceholder(pp, 0x27, out);
                            safeParseTypeConstraint(consumeToken(), token, typeBoolean, SI);
                            return typeBoolean;
                        case E_CONFETTI_BALL:
                            writeCoinAtPlaceholder(pp, 0x28, out);
                            safeParseTypeConstraint(consumeToken(), token, typeBoolean, SI);
                            return typeBoolean;
                    }
                }
                else if (type.type == TT_INTEGER) {
                    switch (token->value[0]) {
                        case E_HEAVY_MINUS_SIGN:
                            writeCoinAtPlaceholder(pp, 0x21, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeInteger;
                        case E_HEAVY_PLUS_SIGN:
                            writeCoinAtPlaceholder(pp, 0x22, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeInteger;
                        case E_HEAVY_DIVISION_SIGN:
                            writeCoinAtPlaceholder(pp, 0x24, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeInteger;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            writeCoinAtPlaceholder(pp, 0x23, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeInteger;
                        case E_LEFT_POINTING_TRIANGLE:
                            writeCoinAtPlaceholder(pp, 0x29, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            writeCoinAtPlaceholder(pp, 0x2A, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            writeCoinAtPlaceholder(pp, 0x2B, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            writeCoinAtPlaceholder(pp, 0x2C, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeBoolean;
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            writeCoinAtPlaceholder(pp, 0x25, out);
                            safeParseTypeConstraint(consumeToken(), token, typeInteger, SI);
                            return typeInteger;
                    }
                }
                else if (type.type == TT_DOUBLE) {
                    switch (token->value[0]) {
                        case E_FACE_WITH_STUCK_OUT_TONGUE:
                            writeCoinAtPlaceholder(pp, 0x2F, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeBoolean;
                        case E_HEAVY_MINUS_SIGN:
                            writeCoinAtPlaceholder(pp, 0x30, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeFloat;
                        case E_HEAVY_PLUS_SIGN:
                            writeCoinAtPlaceholder(pp, 0x31, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeFloat;
                        case E_HEAVY_DIVISION_SIGN:
                            writeCoinAtPlaceholder(pp, 0x33, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeFloat;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            writeCoinAtPlaceholder(pp, 0x32, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeFloat;
                        case E_LEFT_POINTING_TRIANGLE:
                            writeCoinAtPlaceholder(pp, 0x34, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            writeCoinAtPlaceholder(pp, 0x35, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            writeCoinAtPlaceholder(pp, 0x36, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            writeCoinAtPlaceholder(pp, 0x37, out);
                            safeParseTypeConstraint(consumeToken(), token, typeFloat, SI);
                            return typeBoolean;
                    }
                }
                
                if(token->value[0] == E_FACE_WITH_STUCK_OUT_TONGUE){
                    safeParseTypeConstraint(consumeToken(), token, type, SI); //Must be of the same type as the callee
                    writeCoinAtPlaceholder(pp, 0x20, out);
                    return typeBoolean;
                }
                
                ecCharToCharStack(token->value[0], method);
                auto typeString = type.toString(SI->classTypeContext, true);
                compilerError(token, "Unknown primitive method %s for %s.", method, typeString.c_str());
            }
            
            if(method == NULL){
                auto eclass = type.toString(SI->classTypeContext, true);
                ecCharToCharStack(token->value[0], method);
                compilerError(token, "%s has no method %s.", eclass.c_str(), method);
            }
            
            if(type.type == TT_PROTOCOL){
                writeCoinAtPlaceholder(pp, 0x3, out);
                writeCoin(type.protocol->index, out);
                writeCoin(method->vti, out);
            }
            else if(type.type == TT_CLASS) {
                writeCoinAtPlaceholder(pp, 0x1, out);
                writeCoin(method->vti, out);
            }

            checkAccess(method, token, "Method", SI);
            checkArguments(method->arguments, type, token, SI);

            return method->returnType.resolveOn(type);
        }
    }
    return typeNothingness;
}

Type typeParse(Token *token, StaticInformation *SI){
    switch(token->type){
        case STRING: {
            //Instruction to create a string
            writeCoin(0x10, out);
            
            for (size_t i = 0; i < stringPool.size(); i++) {
                Token *a = stringPool[i];
                if (a->value.compare(token->value) == 0) {
                    writeCoin((EmojicodeCoin)i, out);
                    return Type(CL_STRING);
                }
            }
            
            writeCoin((EmojicodeCoin)stringPool.size(), out);
            stringPool.push_back(token);
            
            return Type(CL_STRING);
        } 
        case BOOLEAN_TRUE:
            writeCoin(0x11, out);
            return typeBoolean;
        case BOOLEAN_FALSE:
            writeCoin(0x12, out);
            return typeBoolean;
        case INTEGER: {
            /* We know token->value only contains ints less than 255 */
            const char *string = token->value.utf8CString();
            
            EmojicodeInteger l = strtoll(string, NULL, 0);
            delete [] string;
            if (llabs(l) > INT32_MAX) {
                writeCoin(0x14, out);

                writeCoin((l >> 32), out);
                writeCoin((EmojicodeCoin)l, out);
                
                return typeInteger;
            }
            else {
                writeCoin(0x13, out);
                writeCoin((EmojicodeCoin)l, out);
                
                return typeInteger;
            }
        }
        case DOUBLE: {
            writeCoin(0x15, out);
            
            const char *string = token->value.utf8CString();
            
            double d = strtod(string, NULL);
            delete [] string;
            writeDouble(d, out);
            return typeFloat;
        }
        case SYMBOL:
            writeCoin(0x16, out);
            writeCoin(token->value[0], out);
            return typeSymbol;
        case VARIABLE: {
            uint8_t scopesUp;
            CompilerVariable *cv = getVariable(token, &scopesUp);
            
            if(cv == NULL){
                const char *variableName = token->value.utf8CString();
                compilerError(token, "Variable \"%s\" not defined.", variableName);
            }

            cv->uninitalizedError(token);
            
            writeCoinForScopesUp(scopesUp, token, 0x1A, 0x1C, SI);
            writeCoin(cv->id, out);

            return cv->type;
        }
        case IDENTIFIER:
            return typeParseIdentifier(token, SI);
        case DOCUMENTATION_COMMENT:
            compilerError(token, "Misplaced documentation comment.");
        case NO_TYPE:
        case COMMENT:
            break;
    }
    compilerError(token, "Cannot determine expression’s return type.");
}

void analyzeFunctionBody(Token *firstToken, Arguments arguments, StaticInformation *SI){
    analyzeFunctionBodyFull(firstToken, arguments, SI, false, NULL);
}

void analyzeFunctionBodyFull(Token *firstToken, Arguments arguments, StaticInformation *SI, bool compileDeadCode, Scope *copyScope){
    currentToken = firstToken;
    
    SI->returned = false;
    SI->flowControlDepth = 0;
    SI->variableCount = 0;

    //Set the arguments to the method scope
    Scope methodScope(false);
    for (auto variable : arguments) {
        uint8_t id = nextVariableID(SI);
        CompilerVariable *varo = new CompilerVariable(variable.type, id, true, false);
        
        methodScope.setLocalVariable(variable.name, varo);
    }
    
    if (copyScope) {
        SI->variableCount += methodScope.copyFromScope(copyScope, nextVariableID(SI));
    }
    
    pushScope(&methodScope);
    
    bool emittedDeadCodeWarning = false;
    
    Token *token;
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        SI->effect = false;
        
        typeParse(token, SI);
        
        if(!SI->effect){
            noEffectWarning(token);
        }
        if(!emittedDeadCodeWarning && SI->returned && nextToken()->value[0] != E_WATERMELON && token->type == IDENTIFIER){
            compilerWarning(consumeToken(), "Dead code.");
            emittedDeadCodeWarning = true;
            if (!compileDeadCode) {
                break;
            }
        }
    }
    
    popScope();
}

void checkOverride(void *superWhatsit, bool override, EmojicodeChar name, Token *token){
    if(override && !superWhatsit){
        ecCharToCharStack(name, mn);
        compilerError(token, "%s was declared ✒️ but does not override anything.", mn);
    }
    else if(!override && superWhatsit){
        ecCharToCharStack(name, mn);
        compilerError(token, "If you want to override %s add ✒️.", mn);
    }
}

void analyzeClass(Class *eclass, Type classType){
    writeEmojicodeChar(eclass->name, out);
    if(eclass->superclass){
        writeUInt16(eclass->superclass->index, out);
    }
    else { //If the eclass does not have a superclass the own index gets written
        writeUInt16(eclass->index, out);
    }
    
    Scope objectScope(true);
    
    //Get the ID offset for this eclass by summing up all superclasses instance variable counts
    eclass->IDOffset = 0;
    for(Class *aClass = eclass->superclass; aClass != NULL; aClass = aClass->superclass){
        eclass->IDOffset += aClass->instanceVariables.size();
    }
    writeUInt16(eclass->instanceVariables.size() + eclass->IDOffset, out);
    
    //Number of methods inclusive superclass
    writeUInt16(eclass->nextMethodVti, out);
    //Number of eclass methods inclusive superclass
    writeUInt16(eclass->nextClassMethodVti, out);
    //Initializer inclusive superclass
    fputc(eclass->inheritsContructors, out);
    writeUInt16(eclass->nextInitializerVti, out);
    
    {
        uint16_t offset = eclass->IDOffset;
        for(Class *aClass = eclass; aClass != NULL; aClass = aClass->superclass){
            if(aClass != eclass){
                //If this is not the eclass we are going to analyze we subtract the number of
                //the eclass being anaylzed to get the current classes offset
                offset -= aClass->instanceVariables.size();
            }
        }
        for (auto var : eclass->instanceVariables) {
            CompilerVariable *cv = new CompilerVariable(var->type, offset++, 1, false);
            cv->variable = var;
            objectScope.setLocalVariable(var->name, cv);
        }
    }
    
    StaticInformation *SI = new StaticInformation(eclass);
    SI->inClassContext = false;
    
    pushScope(&objectScope);
    
    writeUInt16(eclass->methodList.size(), out);
    writeUInt16(eclass->initializerList.size(), out);
    writeUInt16(eclass->classMethodList.size(), out);
    
    for (auto method : eclass->methodList) {
        off_t metaPosition;
        if (writeProcedureHeading(method, out, &metaPosition)) continue;
        
        SI->returnType = method->returnType;
        SI->currentNamespace = method->enamespace;
        
        analyzeFunctionBody(method->firstToken, method->arguments, SI);
        noReturnError(method->dToken, SI);
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
    }
    
    for (auto initializer : eclass->initializerList) {
        currentScopeWrapper->scope->changeInitializedBy(-1);

        off_t metaPosition;
        if (writeProcedureHeading(initializer, out, &metaPosition)) continue;
        
        SI->initializer = initializer;
        SI->currentNamespace = initializer->enamespace;
        
        analyzeFunctionBody(initializer->firstToken, initializer->arguments, SI);
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
        
        currentScopeWrapper->scope->initializerUnintializedVariablesCheck(initializer->dToken, "Instance variable \"%s\" must be initialized.");
        
        if (!SI->calledSuper && eclass->superclass) {
            ecCharToCharStack(initializer->name, initializerName);
            compilerError(initializer->dToken, "Missing call to superinitializer in initializer %s.", initializerName);
        }
    }
    
    popScope();
    
    SI->initializer = NULL;
    SI->inClassContext = true;
    
    for (auto classMethod : eclass->classMethodList) {
        off_t metaPosition;
        if (writeProcedureHeading(classMethod, out, &metaPosition)) continue;
        
        SI->returnType = classMethod->returnType;
        SI->currentNamespace = classMethod->enamespace;
        
        analyzeFunctionBody(classMethod->firstToken, classMethod->arguments, SI);
        noReturnError(classMethod->dToken, SI);
        
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
    }
    
    if (eclass->instanceVariables.size() > 0 && eclass->initializerList.size() == 0) {
        ecCharToCharStack(eclass->name, className);
        ecCharToCharStack(eclass->enamespace, classNamespace);
        compilerWarning(eclass->classBegin, "Class %s in %s defines %d instances variables but has no initializers.", className, classNamespace, eclass->instanceVariables.size());
    }
    
    writeUInt16(eclass->protocols.size(), out);
    if (eclass->protocols.size() > 0) {
        off_t position = ftello(out);
        writeUInt16(0, out);
        writeUInt16(0, out);
        
        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;
        
        for(auto protocol : eclass->protocols){
            
            writeUInt16(protocol->index, out);
            
            if(protocol->index > biggestProtocolIndex){
                biggestProtocolIndex = protocol->index;
            }
            if(protocol->index < smallestProtocolIndex){
                smallestProtocolIndex = protocol->index;
            }
            
            writeUInt16(protocol->methodList.size(), out);
            
            for(auto method : protocol->methodList){
                Method *clm = eclass->getMethod(method->name);
                
                if(clm == NULL){
                    ecCharToCharStack(protocol->name, prs);
                    ecCharToCharStack(eclass->name, cls);
                    ecCharToCharStack(method->name, ms);
                    compilerError(eclass->classBegin, "Class %s does not agree to protocol %s: Method %s is missing.", cls, prs, ms);
                }
                
                writeUInt16(clm->vti, out);
                clm->checkPromises(method, "protocol definition of the method", SI->classTypeContext);
            }
        }
        
        off_t oldPosition = ftello(out);
        fseek(out, position, SEEK_SET);
        writeUInt16(biggestProtocolIndex, out);
        writeUInt16(smallestProtocolIndex, out);
        fseek(out, oldPosition, SEEK_SET);
    }
    
    delete SI;
}

void analyzeClassesAndWrite(FILE *fout){
    out = fout;
    
    stringPool.push_back(new Token(NULL));
    
    //Start the writing
    fputc(ByteCodeSpecificationVersion, out); //Version
    
    //Decide which classes inherit initializers, if they agree to protocols, and assign virtual table indexes before we analyze the classes!
    for (auto eclass : classes) {
        //decide whether this eclass is eligible for initializer inheritance
        if(eclass->instanceVariables.size() == 0 && eclass->initializerList.size() == 0){
            eclass->inheritsContructors = true;
        }
        
        if(eclass->superclass){
            eclass->nextClassMethodVti = eclass->superclass->nextClassMethodVti;
            eclass->nextInitializerVti = eclass->inheritsContructors ? eclass->superclass->nextInitializerVti : 0;
            eclass->nextMethodVti = eclass->superclass->nextMethodVti;
        }
        else {
            eclass->nextClassMethodVti = 0;
            eclass->nextInitializerVti = 0;
            eclass->nextMethodVti = 0;
        }
        
        Type classType = Type(eclass);
        
        for(auto method : eclass->methodList){
            Method *superMethod = eclass->superclass->getMethod(method->name);

            checkOverride(superMethod, method->overriding, method->name, method->dToken);
            if (superMethod){
                method->checkPromises(superMethod, "super method", classType);
                method->vti = superMethod->vti;
            }
            else {
                method->vti = eclass->nextMethodVti++;
            }
        }
        for(auto clMethod : eclass->classMethodList){
            ClassMethod *superMethod = eclass->superclass->getClassMethod(clMethod->name);
            
            checkOverride(superMethod, clMethod->overriding, clMethod->name, clMethod->dToken);
            if (superMethod){
                clMethod->checkPromises(superMethod, "super classmethod", classType);
                clMethod->vti = superMethod->vti;
            }
            else {
                clMethod->vti = eclass->nextClassMethodVti++;
            }
        }
        for(auto initializer : eclass->initializerList){ //TODO: heavily incorrect
            Initializer *superConst = eclass->superclass->getInitializer(initializer->name);
            
            checkOverride(superConst, initializer->overriding, initializer->name, initializer->dToken);
            if (superConst){
                initializer->checkPromises(superConst, "super classmethod", classType);
                //if a eclass has a initializer it does not inherit other initializers, therefore inheriting the VTI could have fatal consequences
            }
            initializer->vti = eclass->nextInitializerVti++;
        }
    }
    
    //Write Number of Classes
    writeUInt16(classes.size(), out);
    
    uint8_t pkgCount = (uint8_t)packages.size();
    //must be s and _
    if(pkgCount == 2){
        pkgCount = 1;
    }
    if(pkgCount > 253){
        compilerError(NULL, "You exceeded the maximum of 253 packages.");
    }
    fputc(pkgCount, out);
    
    size_t pkgI = 0;
    
    Package *pkg = NULL;
  
    //Analyze all classes
    for (size_t i = 0; i < classes.size(); i++) {
        Class *eclass = classes[i];
        
        if((pkg != eclass->package && pkgCount > 1) || !pkg){ //pkgCount > 1: Ignore the second s
            if (i > 0){
                fputc(0, out);
            }
            pkg = eclass->package;
            Package *pkg = packages[pkgI++];
            
            uint16_t l = strlen(pkg->name) + 1;
            writeUInt16(l, out);
            
            fwrite(pkg->name, sizeof(char), l, out);
            
            writeUInt16(pkg->version.major, out);
            writeUInt16(pkg->version.minor, out);
            
            fputc(pkg->requiresNativeBinary ? 1 : 0, out);
        }
        else if (i > 0){
            fputc(1, out);
        }
        
        analyzeClass(eclass, Type(eclass));
    }
    fputc(0, out);
    
    writeUInt16(stringPool.size(), out);
    for (auto token : stringPool) {
        writeUInt16(token->value.size(), out);
        
        for (auto c : token->value) {
            writeEmojicodeChar(c, out);
        }
    }
    
    writeUInt16(startingFlag.eclass->index, out);
    writeUInt16(startingFlag.eclass->getClassMethod(E_CHEQUERED_FLAG)->vti, out);
}