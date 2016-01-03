//
//  StaticAnalyzer.c
//  Emojicode
//
//  Created by Theo Weidmann on 05.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <string.h>
#include <limits.h>

#include "StaticAnalyzer.h"
#include "EmojicodeCompiler.h"
#include "Writer.h"
#include "ClassParser.h"
#include "CompilerScope.h"

static FILE *out;
static List *stringPool;

//MARK: Compiler Variables

static CompilerVariable* newCompilerVariableObject(Type type, uint8_t id, bool initd, bool frozen){
    CompilerVariable *var = malloc(sizeof(CompilerVariable));
    var->type = type;
    var->id = id;
    var->initialized = initd;
    var->frozen = frozen;
    return var;
}

static void uninitalizedVariableError(CompilerVariable *var, Token *variableToken){
    if (var->initialized <= 0) {
        String string = {variableToken->valueLength, variableToken->value};
        char *variableName = stringToChar(&string);
        compilerError(variableToken, "Variable \"%s\" is possibly not initialized.", variableName);
    }
}

static void frozenVariableError(CompilerVariable *var, Token *variableToken){
    if (var->frozen) {
        String string = {variableToken->valueLength, variableToken->value};
        char *variableName = stringToChar(&string);
        compilerError(variableToken, "Cannot modify frozen variable \"%s\".", variableName);
    }
}

/** Emits @c errorMessage if not all instance variable were initialized. @c errorMessage should include @c %s for the name of the variable. */
static void initializerUnintializedInstanceVariablesCheck(Scope *instanceScope, Token *errorToken, const char *errorMessage){
    Dictionary *dict = instanceScope->map;
    for (size_t i = 0; i < dict->capacity; i++) {
        if(dict->slots[i].key){
            CompilerVariable *cv = dict->slots[i].value;
            if (cv->initialized <= 0 && !cv->type.optional) {
                String string = {cv->variable->name->valueLength, cv->variable->name->value};
                char *variableName = stringToChar(&string);
                compilerError(errorToken, errorMessage, variableName);
            }
        }
    }
}

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
    if(!typesCompatible(v, type, SI->classTypeContext)){
        char *cn = typeToString(v, SI->classTypeContext, true);
        char *tn = typeToString(type, SI->classTypeContext, true);
        compilerError(token, "%s is not compatible to %s.", cn, tn);
    }
    return v;
}


//MARK: Block utilities

static void changeScope(Scope *scope, int c){
    Dictionary *dict = scope->map;
    for (size_t i = 0; i < dict->capacity; i++) {
        if(dict->slots[i].key){
            CompilerVariable *cv = dict->slots[i].value;
            if (cv->initialized > 0) {
                cv->initialized += c;
            }
        }
    }
}

/** Handles a complete block */
static void block(StaticInformation *SI){
    changeScope(currentScopeWrapper->scope, 1);
    if (!SI->inClassContext) {
        changeScope(currentScopeWrapper->topScope->scope, 1);
    }
    
    SI->flowControlDepth++;
    
    Token *token = consumeToken();
    tokenTypeCheck(IDENTIFIER, token);
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
    changeScope(currentScopeWrapper->scope, -1);
    if (!SI->inClassContext) {
        changeScope(currentScopeWrapper->topScope->scope, -1);
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
    if (!typeIsNothingness(SI->returnType) && !SI->returned) {
        compilerError(errorToken, "An explicit return is missing.");
    }
}

//MARK: Low level parsing

void checkArguments(Arguments arguments, Type calledType, Token *token, StaticInformation *SI){
    for (int i = 0; i < arguments.count; i++) {
        safeParseTypeConstraint(consumeToken(), token, resolveTypeReferences(arguments.variables[i].type, calledType), SI);
    }
}

static void checkAccess(Procedure *p, Token *token, const char *type, StaticInformation *SI){
    if (p->access == PRIVATE) {
        if (p->class != SI->classTypeContext.class) {
            ecCharToCharStack(p->name, nm);
            compilerError(token, "%s %s is 🔒.", type, nm);
        }
    }
    else if(p->access == PROTECTED) {
        if (!inheritsFrom(SI->classTypeContext.class, p->class)) {
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
        tokenTypeCheck(VARIABLE, varName);
        
        if(getLocalVariable(varName, currentScopeWrapper->scope) != NULL){
            compilerError(token, "Cannot redeclare variable.");
        }
        
        uint8_t id = nextVariableID(SI);
        writeCoin(id, out);
        
        Type t = safeParse(consumeToken(), token, SI);
        if (!t.optional) {
            compilerError(token, "🍊🍦 can only be used with optionals.");
        }
        
        t.optional = false;
        setLocalVariable(varName, newCompilerVariableObject(t, id, 1, true), currentScopeWrapper->scope);
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
            tokenTypeCheck(VARIABLE, varName);
            
            if (getLocalVariable(varName, currentScopeWrapper->scope) != NULL) {
                compilerError(token, "Cannot redeclare variable.");
            }
            
            Type t = parseAndFetchType(SI->classTypeContext.class, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            uint8_t id = nextVariableID(SI);
            setLocalVariable(varName, newCompilerVariableObject(t, id, t.optional ? 1 : 0, false), currentScopeWrapper->scope);
            
            return typeNothingness;
        }
        case E_CUSTARD: {
            Token *varName = consumeToken();
            tokenTypeCheck(VARIABLE, varName);
            
            uint8_t scopesUp;
            CompilerVariable *cv = getVariable(varName, &scopesUp);
            if(cv == NULL){
                //Not declared, declaring as local variable

                writeCoin(0x1B, out);
                
                uint8_t id = nextVariableID(SI);
                
                writeCoin(id, out);
                
                Type t = safeParse(consumeToken(), token, SI);
                setLocalVariable(varName, newCompilerVariableObject(t, id, 1, false), currentScopeWrapper->scope);
            }
            else {
                if (cv->initialized <= 0) {
                    cv->initialized = 1;
                }
                
                frozenVariableError(cv, varName);
                
                writeCoinForScopesUp(scopesUp, varName, 0x1B, 0x1D, SI);
                writeCoin(cv->id, out);
                
                safeParseTypeConstraint(consumeToken(), token, cv->type, SI);
            }
            return typeNothingness;
        }
        case E_SOFT_ICE_CREAM: {
            Token *varName = consumeToken();
            tokenTypeCheck(VARIABLE, varName);
            
            if(getLocalVariable(varName, currentScopeWrapper->scope) != NULL){
                compilerError(token, "Cannot redeclare variable.");
            }
            
            
            writeCoin(0x1B, out);
            
            uint8_t id = nextVariableID(SI);
            writeCoin(id, out);
            
            Type t = safeParse(consumeToken(), token, SI);
            setLocalVariable(varName, newCompilerVariableObject(t, id, 1, true), currentScopeWrapper->scope);
            return typeNothingness;
        }
        case E_COOKING:
        case E_CHOCOLATE_BAR: {
            Token *varName = consumeToken();
            tokenTypeCheck(VARIABLE, varName);
            
            //Fetch the old value
            uint8_t scopesUp;
            CompilerVariable *cv = getVariable(varName, &scopesUp);
            
            if (!cv) {
                String string = {token->valueLength, token->value};
                char *variableName = stringToChar(&string);
                compilerError(token, "Unknown variable \"%s\"", variableName);
                break;
            }
            
            uninitalizedVariableError(cv, varName);
            frozenVariableError(cv, varName);
            
            if (!typesCompatible(cv->type, typeInteger, SI->classTypeContext)) {
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
            
            safeParseTypeConstraint(consumeToken(), token, typeForClass(CL_STRING), SI);
            
            Token *stringToken;
            while (stringToken = consumeToken(), !(stringToken->type == IDENTIFIER && stringToken->value[0] == E_COOKIE)) {
                safeParseTypeConstraint(stringToken, token, typeForClass(CL_STRING), SI);
                n++;
            }
            
            writeCoinAtPlaceholder(pp, n, out);
            return typeForClass(CL_STRING);
        }
        case E_ICE_CREAM: {
            writeCoin(0x51, out);
            off_t pp = writePlaceholderCoin(out);
            
            uint32_t delta = writtenCoins;
            
            bool firstTypeFound = false;
            Type commonType;
            
            Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                Type t = safeParse(aToken, token, SI);
                determineCommonType(t, &commonType, &firstTypeFound, SI->classTypeContext);
            }
            
            emitCommonTypeWarning(&commonType, &firstTypeFound, token);
            
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            Type type = typeForClass(CL_LIST);
            type.genericArguments[0] = commonType;
            return type;
        }
        case E_HONEY_POT: {
            writeCoin(0x50, out);
            off_t pp = writePlaceholderCoin(out);
            
            uint32_t delta = writtenCoins;
            
            bool firstTypeFound = false;
            Type commonType = typeSomeobject;
            
            Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                safeParseTypeConstraint(aToken, token, typeForClass(CL_STRING), SI);
                Type t = safeParse(consumeToken(), token, SI);
                determineCommonType(t, &commonType, &firstTypeFound, SI->classTypeContext);
            }
            
            emitCommonTypeWarning(&commonType, &firstTypeFound, token);
            
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            
            Type type = typeForClass(CL_DICTIONARY);
            type.genericArguments[0] = commonType;
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
            tokenTypeCheck(VARIABLE, variableToken);
            
            if (getLocalVariable(variableToken, currentScopeWrapper->scope) != NULL) {
                compilerError(variableToken, "Cannot redeclare variable.");
            }
            
            uint8_t vID = nextVariableID(SI);
            writeCoin(vID, out);
            //Internally needed
            writeCoin(nextVariableID(SI), out);
            
            Type iteratee = safeParseTypeConstraint(consumeToken(), token, typeSomeobject, SI);
            
            if(iteratee.type == TT_CLASS && iteratee.class == CL_LIST) {
                //If the iteratee is a list, the Real-Time Engine has some special sugar
                writeCoinAtPlaceholder(pp, 0x65, out);
                setLocalVariable(variableToken, newCompilerVariableObject(iteratee.genericArguments[0], vID, true, false), currentScopeWrapper->scope);
            }
            else if(typesCompatible(iteratee, typeForProtocol(PR_ENUMERATEABLE), SI->classTypeContext)) {
                writeCoinAtPlaceholder(pp, 0x64, out);
                Type itemType = typeSomething;
                if(iteratee.type == TT_CLASS && iteratee.class->ownGenericArgumentCount == 1) {
                    itemType = iteratee.genericArguments[iteratee.class->ownGenericArgumentCount - iteratee.class->genericArgumentCount];
                }
                setLocalVariable(variableToken, newCompilerVariableObject(itemType, vID, true, false), currentScopeWrapper->scope);
            }
            else {
                char *iterateeString = typeToString(iteratee, SI->classTypeContext, true);
                compilerError(token, "%s does not conform to 🔴🔂.", iterateeString);
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
            if (SI->initializer && !SI->calledSuper && SI->initializer->pc.class->superclass) {
                compilerError(token, "Attempt to use 🐕 before superinitializer call.");
            }
            
            if (SI->inClassContext) {
                compilerError(token, "Illegal use of 🐕.", token->value[0]);
                break;
            }
            
            initializerUnintializedInstanceVariablesCheck(currentScopeWrapper->topScope->scope, token, "Instance variable \"%s\" must be initialized before the use of 🐕.");

            return SI->classTypeContext;
        }
        case E_UP_POINTING_RED_TRIANGLE: {
            writeCoin(0x13, out);
            
            Type type = parseAndFetchType(SI->classTypeContext.class, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            if (type.type != TT_ENUM) {
                compilerError(token, "The given type cannot be accessed.");
            }
            else if (type.optional) {
                compilerError(token, "Optionals cannot be accessed.");
            }
            
            Token *name = consumeToken();
            tokenTypeCheck(IDENTIFIER, name);
            
            EmojicodeInteger *v = enumGetValue(name->value[0], type.eenum);
            if (!v) {
                ecCharToCharStack(name->value[0], valueName);
                ecCharToCharStack(type.eenum->name, enumName);
                compilerError(name, "%s does not have a member named %s.", enumName, valueName);
            }
            else if (*v > UINT32_MAX) {
                writeCoin((*v >> 32), out);
                writeCoin((EmojicodeCoin)*v, out);
            }
            else {
                writeCoin((EmojicodeCoin)*v, out);
            }
            
            return type;
        }
        case E_LARGE_BLUE_DIAMOND: {
            writeCoin(0x4, out);
            
            bool dynamic;
            Type type = parseAndFetchType(SI->classTypeContext.class, SI->currentNamespace, dynamismLevelFromSI(SI), &dynamic);
            
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
                writeCoin(type.class->index, out);
            }
            
            //The initializer name
            Token *consName = consumeToken();
            tokenTypeCheck(IDENTIFIER, consName);
            
            Initializer *initializer = getInitializer(consName->value[0], type.class);
            
            if (initializer == NULL) {
                char *typeString = typeToString(type, SI->classTypeContext, true);
                ecCharToCharStack(consName->value[0], initializerString);
                compilerError(consName, "%s has no initializer %s.", typeString, initializerString);
            }
            else if (dynamic && !initializer->required) {
                compilerError(consName, "Only required initializers can be used with 🐀.");
            }
            
            writeCoin(initializer->pc.vti, out);
            
            checkAccess((Procedure *)initializer, token, "Initializer", SI);
            checkArguments(initializer->pc.arguments, type, token, SI);
            
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
            Token *t = consumeToken();
            tokenTypeCheck(NO_TYPE, t);
            typeParse(t, SI);
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
            if (!SI->classTypeContext.class->superclass) {
                compilerError(token, "🐐 can only be used if the class inherits from another.");
                break;
            }
            if (SI->calledSuper) {
                compilerError(token, "You may not call more than one superinitializer.");
            }
            if (SI->flowControlDepth) {
                compilerError(token, "You may not put a call to a superinitializer in a flow control structure.");
            }
            
            initializerUnintializedInstanceVariablesCheck(currentScopeWrapper->topScope->scope, token, "Instance variable \"%s\" must be initialized before superinitializer.");
            
            writeCoin(0x3D, out);
            
            Class *class = SI->classTypeContext.class;
            
            writeCoin(class->superclass->index, out);
            
            Token *initializerToken = consumeToken();
            tokenTypeCheck(IDENTIFIER, initializerToken);
            
            Initializer *initializer = getInitializer(initializerToken->value[0], class->superclass);
            
            if (initializer == NULL) {
                ecCharToCharStack(initializerToken->value[0], initializerString);
                compilerError(initializerToken, "Cannot find superinitializer %s.", initializerString);
                break;
            }
            
            writeCoin(initializer->pc.vti, out);
            
            checkAccess((Procedure *)initializer, token, "initializer", SI);
            checkArguments(initializer->pc.arguments, SI->classTypeContext, token, SI);

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
            Type type = parseAndFetchType(SI->classTypeContext.class, SI->currentNamespace, dynamismLevelFromSI(SI), &dynamic);
            
            if (dynamic) {
                compilerError(token, "You cannot cast to dynamic types.");
            }
            
            if (typesCompatible(originalType, type, SI->classTypeContext)) {
                compilerWarning(token, "Superfluous cast.");
            }
            
            switch (type.type) {
                case TT_CLASS:
                    for (size_t i = 0; i < type.class->ownGenericArgumentCount; i++) {
                        if(!typesCompatible(type.class->genericArgumentContraints[i], type.genericArguments[i], type) ||
                           !typesCompatible(type.genericArguments[i], type.class->genericArgumentContraints[i], type)) {
                            compilerError(token, "Dynamic casts involving generic type arguments are not possible yet. Please specify the generic argument constraints of the class for compatibility with future versions.");
                        }
                    }

                    writeCoinAtPlaceholder(pp, originalType.type == TT_SOMETHING || originalType.optional ? 0x44 : 0x40, out);
                    writeCoin(type.class->index, out);
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
                    char *typeString = typeToString(type, SI->classTypeContext, true);
                    compilerError(token, "You cannot cast to %s.", typeString);
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
            
            Method *method = getMethod(methodToken->value[0], type.class);
            
            if(method == NULL){
                char *class = typeToString(type, SI->classTypeContext, true);
                ecCharToCharStack(methodToken->value[0], method);
                compilerError(token, "%s has no method %s", class, method);
            }
            
            writeCoin(method->pc.vti, out);
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            
            checkAccess((Procedure *)method, token, "method", SI);
            checkArguments(method->pc.arguments, type, token, SI);
            
            Type returnType = method->pc.returnType;
            returnType.optional = true;
            return returnType;
        }
        case E_DOUGHNUT: {
            writeCoin(0x2, out);
            
            Token *methodToken = consumeToken();
            tokenTypeCheck(IDENTIFIER, methodToken);
            
            Type type = parseAndFetchType(SI->classTypeContext.class, SI->currentNamespace, dynamismLevelFromSI(SI), NULL);
            
            if (type.optional) {
                compilerWarning(token, "Please remove useless 🍬.");
            }
            if (type.type != TT_CLASS) {
                compilerError(token, "The given type is not a class.");
            }
            
            writeCoin(type.class->index, out);
            
            ClassMethod *method = getClassMethod(methodToken->value[0], type.class);
            
            if (method == NULL) {
                char *classString = typeToString(type, SI->classTypeContext, true);
                ecCharToCharStack(methodToken->value[0], methodString);
                compilerError(token, "%s has no class method %s", classString, methodString);
            }
            
            writeCoin(method->pc.vti, out);
            
            checkAccess((Procedure *)method, token, "Class method", SI);
            checkArguments(method->pc.arguments, type, token, SI);
            
            return resolveTypeReferences(method->pc.returnType, type);
        }
        case E_HOT_PEPPER: {
            Token *methodName = consumeToken();
            
            writeCoin(0x71, out);
            
            Type type = safeParse(consumeToken(), token, SI);
            
            Method *method;
            if (type.type != TT_CLASS) {
                compilerError(token, "Only class.");
            }
            method = getMethod(methodName->value[0], type.class);
            
            if (!method) {
                compilerError(token, "Method is non-existent.");
            }
            
            writeCoin(method->pc.vti, out);
            
            Type t;
            t.type = TT_CALLABLE;
            t.arguments = method->pc.arguments.count;
            t.genericArguments = malloc(sizeof(Type) * (t.arguments + 1));
            
            t.genericArguments[0] = method->pc.returnType;
            for (int i = 0; i < method->pc.arguments.count; i++) {
                t.genericArguments[i + 1] = method->pc.arguments.variables[i].type;
            }
            return t;
        }
        case E_GRAPES: {
            writeCoin(0x70, out);
            
            Type t;
            t.type = TT_CALLABLE;
            
            Procedure p;
            p.class = NULL;
            p.namespace = SI->currentNamespace;
            parseArgumentList(&p);
            
            t.optional = false;
            t.arguments = p.arguments.count;
            t.genericArguments = malloc(sizeof(Type) * (t.arguments + 1));
            
            for (int i = 0; i < p.arguments.count; i++) {
                t.genericArguments[i + 1] = p.arguments.variables[i].type;
            }
            
            parseReturnType(t.genericArguments, NULL, SI->currentNamespace);
            
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
            
            analyzeFunctionBodyFull(currentToken, p.arguments, SI, true, closingScope);
            noReturnError(token, SI);
            
            if (!SI->inClassContext) {
                popScope();
            }
            
            writeCoinAtPlaceholder(variableCountPp, SI->variableCount, out);
            writeCoinAtPlaceholder(pp, writtenCoins - delta, out);
            writeCoin((uint32_t)p.arguments.count | (SI->usedSelf ? 1 << 16 : 0), out);
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
            
            Class *superclass = SI->classTypeContext.class->superclass;
            Method *method = getMethod(nameToken->value[0], superclass);
            
            if (!method) {
                compilerError(token, "Method is non-existent.");
            }
            
            writeCoin(0x5, out);
            writeCoin(superclass->index, out);
            writeCoin(method->pc.vti, out);
            
            checkArguments(method->pc.arguments, SI->classTypeContext, token, SI);
            
            return method->pc.returnType;
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
                method = protocolGetMethod(token->value[0], type.protocol);
            }
            else if(type.type == TT_CLASS) {
                method = getMethod(token->value[0], type.class);
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
                char *typeString = typeToString(type, SI->classTypeContext, true);
                compilerError(token, "Unknown primitive method %s for %s.", method, typeString);
            }
            
            if(method == NULL){
                char *class = typeToString(type, SI->classTypeContext, true);
                ecCharToCharStack(token->value[0], method);
                compilerError(token, "%s has no method %s.", class, method);
                break;
            }
            
            if(type.type == TT_PROTOCOL){
                writeCoinAtPlaceholder(pp, 0x3, out);
                writeCoin(type.protocol->index, out);
                writeCoin(method->pc.vti, out);
            }
            else if(type.type == TT_CLASS) {
                writeCoinAtPlaceholder(pp, 0x1, out);
                writeCoin(method->pc.vti, out);
            }

            checkAccess((Procedure *)method, token, "Method", SI);
            checkArguments(method->pc.arguments, type, token, SI);

            return resolveTypeReferences(method->pc.returnType, type);
        }
    }
    return typeNothingness;
}

bool tokenValueEqual(Token *a, Token *b){
    if(a->valueLength != b->valueLength){
        return false;
    }
    
    for (size_t i = 0; i < a->valueLength; i++)
        if (a->value[i] != b->value[i])
            return false;
    
    return true;
}

Type typeParse(Token *token, StaticInformation *SI){
    switch(token->type){
        case STRING: {
            //Instruction to create a string
            writeCoin(0x10, out);
            
            for (size_t i = 0; i < stringPool->count; i++) {
                Token *a = getList(stringPool, i);
                if (tokenValueEqual(a, token)) {
                    writeCoin((EmojicodeCoin)i, out);
                    return typeForClass(CL_STRING);
                }
            }
            
            writeCoin((EmojicodeCoin)stringPool->count, out);
            appendList(stringPool, token);
            
            return typeForClass(CL_STRING);
        } 
        case BOOLEAN_TRUE:
            writeCoin(0x11, out);
            return typeBoolean;
        case BOOLEAN_FALSE:
            writeCoin(0x12, out);
            return typeBoolean;
        case INTEGER: {
            /* We know token->value only contains ints less than 255 */
            char is[token->valueLength + 1];
            for(uint32_t i = 0; i < token->valueLength; i++){
                is[i] = token->value[i];
            }
            is[token->valueLength] = 0;
            
            EmojicodeInteger l = strtoll(is, NULL, 0);
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
            char is[token->valueLength + 1];
            for (uint32_t i = 0; i < token->valueLength; i++) {
                is[i] = token->value[i];
            }
            is[token->valueLength] = 0;
            
            double d = strtod(is, NULL);
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
                String string = {token->valueLength, token->value};
                char *variableName = stringToChar(&string);
                compilerError(token, "Variable \"%s\" not defined.", variableName);
                break;
            }

            uninitalizedVariableError(cv, token);
            
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
    Scope *methodScope = newSubscope(false);
    for (uint8_t i = 0; i < arguments.count; i++) {
        uint8_t id = nextVariableID(SI);
        CompilerVariable *varo = newCompilerVariableObject(arguments.variables[i].type, id, true, false);
        
        setLocalVariable(arguments.variables[i].name, varo, methodScope);
    }
    if (copyScope) {
        uint8_t offsetID = nextVariableID(SI);
        size_t i = 0;
        for (; i < copyScope->map->capacity; i++) {
            if(copyScope->map->slots[i].key){
                CompilerVariable *ovaro = copyScope->map->slots[i].value;
                CompilerVariable *varo = newCompilerVariableObject(ovaro->type, offsetID + ovaro->id, ovaro->initialized, true);
                
                dictionarySet(methodScope->map, copyScope->map->slots[i].key, copyScope->map->slots[i].kl, varo);
            }
        }
        SI->variableCount += i - 1;
    }
    
    pushScope(methodScope);
    
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
    releaseScope(methodScope);
}

StaticInformation* newStaticInformation(Class *class){
    StaticInformation *si = malloc(sizeof(StaticInformation));
    si->flowControlDepth = 0;
    si->classTypeContext = typeForClass(class);
    si->initializer = NULL;
    return si;
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

void analyzeClass(Class *class, Type classType){
    writeEmojicodeChar(class->name, out);
    if(class->superclass){
        writeUInt16(class->superclass->index, out);
    }
    else { //If the class does not have a superclass the own index gets written
        writeUInt16(class->index, out);
    }
    
    Scope *objectScope = newSubscope(true);
    
    //Get the ID offset for this class by summing up all superclasses instance variable counts
    class->IDOffset = 0;
    for(Class *aClass = class->superclass; aClass != NULL; aClass = aClass->superclass){
        class->IDOffset += aClass->instanceVariableCount;
    }
    writeUInt16(class->instanceVariableCount + class->IDOffset, out);
    
    //Number of methods inclusive superclass
    writeUInt16(class->nextMethodVti, out);
    //Number of class methods inclusive superclass
    writeUInt16(class->nextClassMethodVti, out);
    //Initializer inclusive superclass
    fputc(class->inheritsContructors, out);
    writeUInt16(class->nextInitializerVti, out);
    
    {
        uint16_t offset = class->IDOffset;
        for(Class *aClass = class; aClass != NULL; aClass = aClass->superclass){
            if(aClass != class){
                //If this is not the class we are going to analyze we subtract the number of
                //the class being anaylzed to get the current classes offset
                offset -= aClass->instanceVariableCount;
            }
        }
        for (int i = 0; i < class->instanceVariableCount; i++) {
            Variable *var = class->instanceVariables[i];
            
            CompilerVariable *cv = newCompilerVariableObject(var->type, offset++, 1, false);
            cv->variable = var;
            setLocalVariable(var->name, cv, objectScope);
        }
    }
    
    StaticInformation *SI = newStaticInformation(class);
    SI->inClassContext = false;
    
    pushScope(objectScope);
    
    writeUInt16(class->methodList->count, out);
    writeUInt16(class->initializerList->count, out);
    writeUInt16(class->classMethodList->count, out);
    
    for (uint16_t i = 0; i < class->methodList->count; i++) {
        Method *method = getList(class->methodList, i);
        
        off_t metaPosition;
        if (writeProcedureHeading((Procedure *)method, out, &metaPosition)) continue;
        
        SI->returnType = method->pc.returnType;
        SI->currentNamespace = method->pc.namespace;
        
        analyzeFunctionBody(method->pc.firstToken, method->pc.arguments, SI);
        noReturnError(method->pc.dToken, SI);
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
    }
    
    for (uint16_t i = 0; i < class->initializerList->count; i++) {
        changeScope(currentScopeWrapper->scope, -1);
        Initializer *initializer = getList(class->initializerList, i);

        off_t metaPosition;
        if (writeProcedureHeading((Procedure *)initializer, out, &metaPosition)) continue;
        
        SI->initializer = initializer;
        SI->currentNamespace = initializer->pc.namespace;
        
        analyzeFunctionBody(initializer->pc.firstToken, initializer->pc.arguments, SI);
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
        
        initializerUnintializedInstanceVariablesCheck(currentScopeWrapper->scope, initializer->pc.dToken, "Instance variable \"%s\" must be initialized.");
        
        if (!SI->calledSuper && class->superclass) {
            ecCharToCharStack(initializer->pc.name, initializerName);
            compilerError(initializer->pc.dToken, "Missing call to superinitializer in initializer %s.", initializerName);
        }
    }
    
    popScope();
    
    SI->initializer = NULL;
    SI->inClassContext = true;
    
    for (uint16_t i = 0; i < class->classMethodList->count; i++) {
        ClassMethod *classMethod = getList(class->classMethodList, i);
        
        off_t metaPosition;
        if (writeProcedureHeading((Procedure *)classMethod, out, &metaPosition)) continue;
        
        SI->returnType = classMethod->pc.returnType;
        SI->currentNamespace = classMethod->pc.namespace;
        
        analyzeFunctionBody(classMethod->pc.firstToken, classMethod->pc.arguments, SI);
        noReturnError(classMethod->pc.dToken, SI);
        
        writeFunctionBlockMeta(metaPosition, writtenCoins, SI->variableCount, out);
    }
    
    if (class->instanceVariableCount && !class->initializerList->count) {
        ecCharToCharStack(class->name, className);
        ecCharToCharStack(class->namespace, classNamespace);
        compilerWarning(class->classBegin, "Class %s in %s defines %d instances variables but has no initializers.", className, classNamespace, class->instanceVariableCount);
    }
    
    writeUInt16(class->protocols->count, out);
    ecCharToCharStack(class->name, className);
    if (class->protocols->count > 0) {
        off_t position = ftello(out);
        writeUInt16(0, out);
        writeUInt16(0, out);
        
        uint_fast16_t smallestProtocolIndex = UINT_FAST16_MAX;
        uint_fast16_t biggestProtocolIndex = 0;
        
        for(size_t i = 0; i < class->protocols->count; i++){
            Protocol *protocol = getList(class->protocols, i);
            
            writeUInt16(protocol->index, out);
            
            if(protocol->index > biggestProtocolIndex){
                biggestProtocolIndex = protocol->index;
            }
            if(protocol->index < smallestProtocolIndex){
                smallestProtocolIndex = protocol->index;
            }
            
            writeUInt16(protocol->methodList->count, out);
            
            for(size_t j = 0; j < protocol->methodList->count; j++){
                Method *method = getList(protocol->methodList, j);
                
                Method *clm = getMethod(method->pc.name, class);
                
                if(clm == NULL){
                    ecCharToCharStack(protocol->name, prs);
                    ecCharToCharStack(class->name, cls);
                    ecCharToCharStack(method->pc.name, ms);
                    compilerError(class->classBegin, "Class %s does not agree to protocol %s: Method %s is missing.", cls, prs, ms);
                }
                
                writeUInt16(clm->pc.vti, out);
                checkPromises((Procedure *)clm, (Procedure *)method, "protocol definition of the method", SI->classTypeContext);
            }
        }
        
        off_t oldPosition = ftello(out);
        fseek(out, position, SEEK_SET);
        writeUInt16(biggestProtocolIndex, out);
        writeUInt16(smallestProtocolIndex, out);
        fseek(out, oldPosition, SEEK_SET);
    }
    
    free(SI);
}

void analyzeClassesAndWrite(FILE *fout){
    out = fout;
    stringPool = newList();
    
    Token *token = newToken(NULL);
    token->valueLength = 0;
    appendList(stringPool, token);
    
    //Start the writing
    fputc(ByteCodeSpecificationVersion, out); //Version
    
    //Decide which classes inherit initializers, if they agree to protocols, and assign virtual table indexes before we analyze the classes!
    for (size_t i = 0; i < classes->count; i++) {
        Class *class = getList(classes, i);
        
        //decide whether this class is eligible for initializer inheritance
        if(class->instanceVariableCount == 0 && class->initializerList->count == 0){
            class->inheritsContructors = true;
        }
        //If there are no instance variables we don't need an array to hold them
        if(class->instanceVariableCount == 0){
            free(class->instanceVariables);
        }
        
        if(class->superclass){
            class->nextClassMethodVti = class->superclass->nextClassMethodVti;
            class->nextInitializerVti = class->inheritsContructors ? class->superclass->nextInitializerVti : 0;
            class->nextMethodVti = class->superclass->nextMethodVti;
        }
        else {
            class->nextClassMethodVti = 0;
            class->nextInitializerVti = 0;
            class->nextMethodVti = 0;
        }
        
        Type classType = typeForClass(class);
        
        for(size_t i = 0; i < class->methodList->count; i++){
            Method *method = getList(class->methodList, i);
            Method *superMethod = getMethod(method->pc.name, class->superclass);
            
            checkOverride(superMethod, method->pc.overriding, method->pc.name, method->pc.dToken);
            if (superMethod){
                checkPromises((Procedure *)method, (Procedure *)superMethod, "super method", classType);
                method->pc.vti = superMethod->pc.vti;
            }
            else {
                method->pc.vti = class->nextMethodVti++;
            }
        }
        for(size_t i = 0; i < class->classMethodList->count; i++){
            ClassMethod *clMethod = getList(class->classMethodList, i);
            ClassMethod *superMethod = getClassMethod(clMethod->pc.name, class->superclass);
            
            checkOverride(superMethod, clMethod->pc.overriding, clMethod->pc.name, clMethod->pc.dToken);
            if (superMethod){
                checkPromises((Procedure *)clMethod, (Procedure *)superMethod, "super classmethod", classType);
                clMethod->pc.vti = superMethod->pc.vti;
            }
            else {
                clMethod->pc.vti = class->nextClassMethodVti++;
            }
        }
        for(size_t i = 0; i < class->initializerList->count; i++){ //TODO: heavily incorrect
            Initializer *initializer = getList(class->initializerList, i);
            Initializer *superConst = getInitializer(initializer->pc.name, class->superclass);
            
            checkOverride(superConst, initializer->pc.overriding, initializer->pc.name, initializer->pc.dToken);
            if (superConst){
                checkPromises((Procedure *)initializer, (Procedure *)superConst, "super classmethod", classType);
                //if a class has a initializer it does not inherit other initializers, therefore inheriting the VTI could have fatal consequences
            }
            initializer->pc.vti = class->nextInitializerVti++;
        }
    }
    
    //Write Number of Classes
    writeUInt16(classes->count, out);
    
    uint8_t pkgCount = (uint8_t)packages->count;
    //must be two s
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
    for (size_t i = 0; i < classes->count; i++) {
        Class *class = getList(classes, i);
        
        if((pkg != class->package && pkgCount > 1) || !pkg){ //pkgCount > 1: Ignore the second s
            if (i > 0){
                fputc(0, out);
            }
            pkg = class->package;
            Package *pkg = getList(packages, pkgI++);
            
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
        
        analyzeClass(class, typeForClass(class));
    }
    fputc(0, out);
    
    writeUInt16(stringPool->count, out);
    for (uint16_t i = 0; i < stringPool->count; i++) {
        Token *token = getList(stringPool, i);
        writeUInt16(token->valueLength, out);
        
        for (uint16_t j = 0; j < token->valueLength; j++) {
            writeEmojicodeChar(token->value[j], out);
        }
    }
    
    writeUInt16(startingFlag.class->index, out);
    uint16_t fvti = getClassMethod(E_CHEQUERED_FLAG, startingFlag.class)->pc.vti;
    writeUInt16(fvti, out);
}