//
//  StaticFunctionAnalyzer.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "StaticFunctionAnalyzer.hpp"
#include "FileParser.hpp"
#include "Lexer.hpp"
#include "Type.hpp"
#include "utf8.h"
#include "Class.hpp"

std::vector<const Token *> stringPool;

#define intelligentDynamismLevel() (inClassContext ? TypeDynamism(AllowDynamicClassType | AllowGenericTypeVariables) : AllowGenericTypeVariables)

Type StaticFunctionAnalyzer::parse(const Token *token, const Token *parentToken, Type type) {
    auto returnType = parse(token, parentToken);
    if (!returnType.compatibleTo(type, typeContext)) {
        auto cn = returnType.toString(typeContext, true);
        auto tn = type.toString(typeContext, true);
        compilerError(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

void StaticFunctionAnalyzer::noReturnError(const Token *errorToken) {
    if (callable.returnType.type() != TT_NOTHINGNESS && !returned) {
        compilerError(errorToken, "An explicit return is missing.");
    }
}

void StaticFunctionAnalyzer::noEffectWarning(const Token *warningToken) {
    if (!effect) {
        compilerWarning(warningToken, "Statement seems to have no effect whatsoever.");
    }
}

void StaticFunctionAnalyzer::checkAccess(Procedure *p, const Token *token, const char *type) {
    if (p->access == PRIVATE) {
        if (typeContext.normalType.type() != TT_CLASS || p->eclass != typeContext.normalType.eclass) {
            ecCharToCharStack(p->name, nm);
            compilerError(token, "%s %s is 🔒.", type, nm);
        }
    }
    else if (p->access == PROTECTED) {
        if (typeContext.normalType.type() != TT_CLASS || !typeContext.normalType.eclass->inheritsFrom(p->eclass)) {
            ecCharToCharStack(p->name, nm);
            compilerError(token, "%s %s is 🔐.", type, nm);
        }
    }
}

std::vector<Type> StaticFunctionAnalyzer::checkGenericArguments(Procedure *p, const Token *token) {
    std::vector<Type> k;
    
    while (nextToken()->type == IDENTIFIER && nextToken()->value[0] == E_SPIRAL_SHELL) {
        consumeToken();
        
        auto type = Type::parseAndFetchType(typeContext, intelligentDynamismLevel(), package);
        k.push_back(type);
    }
    
    if (k.size() != p->genericArgumentVariables.size()) {
        compilerError(token, "Too few generic arguments provided.");
    }
    
    return k;
}

void StaticFunctionAnalyzer::checkArguments(Arguments arguments, TypeContext calledType, const Token *token) {
    bool brackets = false;
    if (nextToken()->type == ARGUMENT_BRACKET_OPEN) {
        consumeToken();
        brackets = true;
    }
    for (auto var : arguments) {
        parse(consumeToken(), token, var.type.resolveOn(calledType));
    }
    if (brackets) {
        consumeToken(ARGUMENT_BRACKET_CLOSE);
    }
}

void StaticFunctionAnalyzer::writeCoinForScopesUp(uint8_t scopesUp, const Token *varName, EmojicodeCoin stack, EmojicodeCoin object) {
    if (scopesUp == 0) {
        writer.writeCoin(stack);
    }
    else if (scopesUp == 1) {
        writer.writeCoin(object);
        usedSelf = true;
    }
    else {
        compilerError(varName, "The variable cannot be resolved correctly.");
    }
}

uint8_t StaticFunctionAnalyzer::nextVariableID() {
    return variableCount++;
}

void StaticFunctionAnalyzer::flowControlBlock() {
    scoper.currentScope()->changeInitializedBy(1);
    if (!inClassContext) {
        scoper.topScope()->changeInitializedBy(1);
    }
    
    flowControlDepth++;
    
    const Token *token = consumeToken(IDENTIFIER);
    if (token->value[0] != E_GRAPES) {
        ecCharToCharStack(token->value[0], s);
        compilerError(token, "Expected 🍇 but found %s instead.", s);
    }
    
    auto placeholder = writer.writeCoinsCountPlaceholderCoin();
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        effect = false;
        parse(token, token);
        noEffectWarning(token);
    }
    placeholder.write();
    
    effect = true;
    
    scoper.currentScope()->changeInitializedBy(-1);
    if (!inClassContext) {
        scoper.topScope()->changeInitializedBy(-1);
    }
    
    flowControlDepth--;
}

void StaticFunctionAnalyzer::flowControlReturnEnd(FlowControlReturn &fcr) {
    if (returned) {
        fcr.branchReturns++;
        returned = false;
    }
    fcr.branches++;
}

void StaticFunctionAnalyzer::parseIfExpression(const Token *token) {
    if (nextToken()->value[0] == E_SOFT_ICE_CREAM) {
        consumeToken();
        writer.writeCoin(0x3E);
        
        const Token *varName = consumeToken(VARIABLE);
        if (scoper.currentScope()->getLocalVariable(varName) != nullptr) {
            compilerError(token, "Cannot redeclare variable.");
        }
        
        uint8_t id = nextVariableID();
        writer.writeCoin(id);
        
        Type t = parse(consumeToken(), token);
        if (!t.optional) {
            compilerError(token, "🍊🍦 can only be used with optionals.");
        }
        
        t.optional = false;
        scoper.currentScope()->setLocalVariable(varName, new CompilerVariable(t, id, 1, true, varName));
    }
    else {
        parse(consumeToken(), token, typeBoolean);
    }
}

Type StaticFunctionAnalyzer::parse(const Token *token, const Token *parentToken) {
    if (token == nullptr) {
        compilerError(parentToken, "Unexpected end of function body.");
    }
    
    switch (token->type) {
        case STRING: {
            writer.writeCoin(0x10);
            
            for (size_t i = 0; i < stringPool.size(); i++) {
                const Token *a = stringPool[i];
                if (a->value.compare(token->value) == 0) {
                    writer.writeCoin((EmojicodeCoin)i);
                    return Type(CL_STRING);
                }
            }
            
            writer.writeCoin((EmojicodeCoin)stringPool.size());
            stringPool.push_back(token);
            
            return Type(CL_STRING);
        }
        case BOOLEAN_TRUE:
            writer.writeCoin(0x11);
            return typeBoolean;
        case BOOLEAN_FALSE:
            writer.writeCoin(0x12);
            return typeBoolean;
        case INTEGER: {
            /* We know token->value only contains ints less than 255 */
            const char *string = token->value.utf8CString();
            
            EmojicodeInteger l = strtoll(string, nullptr, 0);
            delete [] string;
            if (llabs(l) > INT32_MAX) {
                writer.writeCoin(0x14);
                
                writer.writeCoin(l >> 32);
                writer.writeCoin((EmojicodeCoin)l);
                
                return typeInteger;
            }
            else {
                writer.writeCoin(0x13);
                writer.writeCoin((EmojicodeCoin)l);
                
                return typeInteger;
            }
        }
        case DOUBLE: {
            writer.writeCoin(0x15);
            
            const char *string = token->value.utf8CString();
            
            double d = strtod(string, nullptr);
            delete [] string;
            writer.writeDouble(d);
            return typeFloat;
        }
        case SYMBOL:
            writer.writeCoin(0x16);
            writer.writeCoin(token->value[0]);
            return typeSymbol;
        case VARIABLE: {
            uint8_t scopesUp;
            CompilerVariable *cv = scoper.getVariable(token, &scopesUp);
            
            if (cv == nullptr) {
                const char *variableName = token->value.utf8CString();
                compilerError(token, "Variable \"%s\" not defined.", variableName);
            }
            
            cv->uninitalizedError(token);
            
            writeCoinForScopesUp(scopesUp, token, 0x1A, 0x1C);
            writer.writeCoin(cv->id);
            
            return cv->type;
        }
        case IDENTIFIER:
            return unsafeParseIdentifier(token);
        case DOCUMENTATION_COMMENT:
            compilerError(token, "Misplaced documentation comment.");
        case ARGUMENT_BRACKET_OPEN:
            compilerError(token, "Unexpected 〖");
        case ARGUMENT_BRACKET_CLOSE:
            compilerError(token, "Unexpected 〗");
        case NO_TYPE:
        case COMMENT:
            break;
    }
    compilerError(token, "Cannot determine expression’s return type.");
}

Type StaticFunctionAnalyzer::unsafeParseIdentifier(const Token *token) {
    if (token->value[0] != E_RED_APPLE) {
        // We need a chance to test whether the red apple’s return is used
        effect = true;
    }

    switch (token->value[0]) {
        case E_SHORTCAKE: {
            const Token *varName = consumeToken(VARIABLE);
            
            if (scoper.currentScope()->getLocalVariable(varName) != nullptr) {
                compilerError(token, "Cannot redeclare variable.");
            }
            
            Type t = Type::parseAndFetchType(typeContext, intelligentDynamismLevel(), package);
            
            uint8_t id = nextVariableID();
            scoper.currentScope()->setLocalVariable(varName,
                                                    new CompilerVariable(t, id, t.optional ? 1 : 0, false, varName));
            
            return typeNothingness;
        }
        case E_CUSTARD: {
            const Token *varName = consumeToken(VARIABLE);
            
            uint8_t scopesUp;
            CompilerVariable *cv = scoper.getVariable(varName, &scopesUp);
            if (cv == nullptr) {
                // Not declared, declaring as local variable
                writer.writeCoin(0x1B);
                
                uint8_t id = nextVariableID();
                
                writer.writeCoin(id);
                
                Type t = parse(consumeToken(), token);
                scoper.currentScope()->setLocalVariable(varName, new CompilerVariable(t, id, 1, false, varName));
            }
            else {
                if (cv->initialized <= 0) {
                    cv->initialized = 1;
                }
                
                cv->mutate(varName);
                
                writeCoinForScopesUp(scopesUp, varName, 0x1B, 0x1D);
                writer.writeCoin(cv->id);
                
                parse(consumeToken(), token, cv->type);
            }
            return typeNothingness;
        }
        case E_SOFT_ICE_CREAM: {
            const Token *varName = consumeToken(VARIABLE);
            
            if (scoper.currentScope()->getLocalVariable(varName) != nullptr) {
                compilerError(token, "Cannot redeclare variable.");
            }
            
            writer.writeCoin(0x1B);
            
            uint8_t id = nextVariableID();
            writer.writeCoin(id);
            
            Type t = parse(consumeToken(), token);
            scoper.currentScope()->setLocalVariable(varName, new CompilerVariable(t, id, 1, true, varName));
            return typeNothingness;
        }
        case E_COOKING:
        case E_CHOCOLATE_BAR: {
            const Token *varName = consumeToken(VARIABLE);
            
            uint8_t scopesUp;
            CompilerVariable *cv = scoper.getVariable(varName, &scopesUp);
            
            if (!cv) {
                compilerError(token, "Unknown variable \"%s\"", varName->value.utf8CString());
                break;
            }
            
            cv->uninitalizedError(varName);
            cv->mutate(varName);
            
            if (!cv->type.compatibleTo(typeInteger, typeContext)) {
                ecCharToCharStack(token->value[0], ls);
                compilerError(token, "%s can only operate on 🚂 variables.", ls);
            }
            
            if (token->value[0] == E_COOKING) {
                writeCoinForScopesUp(scopesUp, varName, 0x19, 0x1F);
            }
            else {
                writeCoinForScopesUp(scopesUp, varName, 0x18, 0x1E);
            }
            
            writer.writeCoin(cv->id);
            
            return typeNothingness;
        }
        case E_COOKIE: {
            writer.writeCoin(0x52);
            auto placeholder = writer.writeCoinPlaceholder();
            
            uint32_t stringCount = 1;
            
            parse(consumeToken(), token, Type(CL_STRING));
            
            const Token *stringToken;
            while (stringToken = consumeToken(), !(stringToken->type == IDENTIFIER && stringToken->value[0] == E_COOKIE)) {
                parse(stringToken, token, Type(CL_STRING));
                stringCount++;
            }
            
            placeholder.write(stringCount);
            return Type(CL_STRING);
        }
        case E_ICE_CREAM: {
            writer.writeCoin(0x51);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            CommonTypeFinder ct;
            
            const Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                ct.addType(parse(aToken, token), typeContext);
            }
            
            placeholder.write();
            
            Type type = Type(CL_LIST);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_HONEY_POT: {
            writer.writeCoin(0x50);
           
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            CommonTypeFinder ct;
            
            const Token *aToken;
            while (aToken = consumeToken(), !(aToken->type == IDENTIFIER && aToken->value[0] == E_AUBERGINE)) {
                parse(aToken, token, Type(CL_STRING));
                ct.addType(parse(consumeToken(), token), typeContext);
            }
            
            placeholder.write();
            
            Type type = Type(CL_DICTIONARY);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE: {
            writer.writeCoin(0x53);
            parse(consumeToken(), token, typeInteger);
            parse(consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            writer.writeCoin(0x54);
            parse(consumeToken(), token, typeInteger);
            parse(consumeToken(), token, typeInteger);
            parse(consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_TANGERINE: {
            writer.writeCoin(0x62);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            auto fcr = FlowControlReturn();
            
            parseIfExpression(token);
            
            flowControlBlock();
            flowControlReturnEnd(fcr);
            
            while ((token = nextToken()) != nullptr && token->type == IDENTIFIER && token->value[0] == E_LEMON) {
                writer.writeCoin(consumeToken()->value[0]);
                
                parseIfExpression(token);
                flowControlBlock();
                flowControlReturnEnd(fcr);
            }
            
            if ((token = nextToken()) != nullptr && token->type == IDENTIFIER && token->value[0] == E_STRAWBERRY) {
                writer.writeCoin(consumeToken()->value[0]);
                flowControlBlock();
                flowControlReturnEnd(fcr);
            }
            else {
                fcr.branches++;  // The else branch always exists. Theoretically at least.
            }
            
            placeholder.write();
            
            returned = fcr.returned();
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
            writer.writeCoin(0x61);
            
            parse(consumeToken(), token, typeBoolean);
            flowControlBlock();
            returned = false;
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            auto placeholder = writer.writeCoinPlaceholder();
            
            const Token *variableToken = consumeToken(VARIABLE);
            
            if (scoper.currentScope()->getLocalVariable(variableToken) != nullptr) {
                compilerError(variableToken, "Cannot redeclare variable.");
            }
            
            uint8_t vID = nextVariableID();
            writer.writeCoin(vID);
            
            Type iteratee = parse(consumeToken(), token, typeSomeobject);
            
            if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_LIST) {
                // If the iteratee is a list, the Real-Time Engine has some special sugar
                placeholder.write(0x65);
                writer.writeCoin(nextVariableID());  //Internally needed
                scoper.currentScope()->setLocalVariable(variableToken, new CompilerVariable(iteratee.genericArguments[0], vID, true, true, variableToken));
            }
            else if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_RANGE) {
                // If the iteratee is a range, the Real-Time Engine also has some special sugar
                placeholder.write(0x66);
                scoper.currentScope()->setLocalVariable(variableToken, new CompilerVariable(typeInteger, vID, true, true, variableToken));
            }
            else if (iteratee.compatibleTo(Type(PR_ENUMERATEABLE, false), typeContext)) {
                placeholder.write(0x64);
                writer.writeCoin(nextVariableID());  //Internally needed
                Type iterator = iteratee.eclass->lookupMethod(E_DANGO)->returnType.resolveOn(iteratee);
                Type itemType = iterator.eclass->lookupMethod(E_DOWN_POINTING_SMALL_RED_TRIANGLE)->returnType.resolveOn(iterator);
                scoper.currentScope()->setLocalVariable(variableToken, new CompilerVariable(itemType, vID, true, true, variableToken));
            }
            else {
                auto iterateeString = iteratee.toString(typeContext, true);
                compilerError(token, "%s does not conform to 🔴🔂.", iterateeString.c_str());
            }
            
            flowControlBlock();
            returned = false;
            
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
            usedSelf = true;
            writer.writeCoin(0x3C);
            if (initializer && !calledSuper && initializer->eclass->superclass) {
                compilerError(token, "Attempt to use 🐕 before superinitializer call.");
            }
            
            if (inClassContext) {
                compilerError(token, "Illegal use of 🐕.", token->value[0]);
                break;
            }
            
            scoper.topScope()->initializerUnintializedVariablesCheck(token,
                                                "Instance variable \"%s\" must be initialized before the use of 🐕.");
            
            return typeContext.normalType;
        }
        case E_UP_POINTING_RED_TRIANGLE: {
            writer.writeCoin(0x13);
            
            Type type = Type::parseAndFetchType(typeContext, intelligentDynamismLevel(), package);
            
            if (type.type() != TT_ENUM) {
                compilerError(token, "The given type cannot be accessed.");
            }
            else if (type.optional) {
                compilerError(token, "Optionals cannot be accessed.");
            }
            
            auto name = consumeToken(IDENTIFIER);
            
            auto v = type.eenum->getValueFor(name->value[0]);
            if (!v.first) {
                ecCharToCharStack(name->value[0], valueName);
                ecCharToCharStack(type.eenum->name(), enumName);
                compilerError(name, "%s does not have a member named %s.", enumName, valueName);
            }
            else if (v.second > UINT32_MAX) {
                writer.writeCoin(v.second >> 32);
                writer.writeCoin((EmojicodeCoin)v.second);
            }
            else {
                writer.writeCoin((EmojicodeCoin)v.second);
            }
            
            return type;
        }
        case E_LARGE_BLUE_DIAMOND: {
            writer.writeCoin(0x4);
            
            bool dynamic;
            Type type = Type::parseAndFetchType(typeContext, intelligentDynamismLevel(), package, &dynamic);
            
            if (type.type() != TT_CLASS) {
                compilerError(token, "The given type cannot be initiatied.");
            }
            else if (type.optional) {
                compilerError(token, "Optionals cannot be initiatied.");
            }
            
            if (dynamic) {
                writer.writeCoin(UINT32_MAX);
            }
            else {
                writer.writeCoin(type.eclass->index);
            }
            
            auto initializerName = consumeToken(IDENTIFIER);
            
            Initializer *initializer = type.eclass->getInitializer(initializerName, type, typeContext);
            
            if (dynamic && !initializer->required) {
                compilerError(initializerName, "Only required initializers can be used with 🐀.");
            }
            
            initializer->deprecatedWarning(initializerName);
            
            writer.writeCoin(initializer->vti);
            
            checkAccess(initializer, token, "Initializer");
            checkArguments(initializer->arguments, type, token);
            
            if (initializer->canReturnNothingness) {
                type.optional = true;
            }
            return type;
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writer.writeCoin(0x17);
            return typeNothingness;
        }
        case E_CLOUD: {
            writer.writeCoin(0x2E);
            parse(consumeToken(), token);
            return typeBoolean;
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writer.writeCoin(0x2D);
            
            parse(consumeToken(), token, typeSomeobject);
            parse(consumeToken(), token, typeSomeobject);
            
            return typeBoolean;
        }
        case E_GOAT: {
            if (!initializer) {
                compilerError(token, "🐐 can only be used inside initializers.");
                break;
            }
            if (!typeContext.normalType.eclass->superclass) {
                compilerError(token, "🐐 can only be used if the eclass inherits from another.");
                break;
            }
            if (calledSuper) {
                compilerError(token, "You may not call more than one superinitializer.");
            }
            if (flowControlDepth) {
                compilerError(token, "You may not put a call to a superinitializer in a flow control structure.");
            }
            
            scoper.topScope()->initializerUnintializedVariablesCheck(token,
                                            "Instance variable \"%s\" must be initialized before superinitializer.");
            
            writer.writeCoin(0x3D);
            
            Class *eclass = typeContext.normalType.eclass;
            
            writer.writeCoin(eclass->superclass->index);
            
            const Token *initializerToken = consumeToken(IDENTIFIER);
            
            auto initializer = eclass->superclass->getInitializer(initializerToken, eclass, typeContext);
            initializer->deprecatedWarning(initializerToken);
            
            writer.writeCoin(initializer->vti);
            
            checkAccess(initializer, token, "initializer");
            checkArguments(initializer->arguments, typeContext.normalType, token);
            
            calledSuper = true;
            
            return typeNothingness;
        }
        case E_RED_APPLE: {
            if (effect) {
                // Effect is true, so apple is called as subcommand
                compilerError(token, "🍎’s return may not be used as an argument.");
            }
            effect = true;
            
            writer.writeCoin(0x60);
            
            if (initializer) {
                if (initializer->canReturnNothingness) {
                    parse(consumeToken(), token, typeNothingness);
                    return typeNothingness;
                }
                else {
                    compilerError(token, "🍎 cannot be used inside a initializer.");
                }
            }
            
            parse(consumeToken(), token, callable.returnType);
            returned = true;
            return typeNothingness;
        }
        case E_BLACK_SQUARE_BUTTON: {
            auto placeholder = writer.writeCoinPlaceholder();
            
            Type originalType = parse(consumeToken(), token, typeSomething);
            Type type = Type::parseAndFetchType(typeContext, NoDynamism, package, nullptr);
            
            if (originalType.compatibleTo(type, typeContext)) {
                compilerWarning(token, "Superfluous cast.");
            }
            
            switch (type.type()) {
                case TT_CLASS: {
                    auto offset = type.eclass->numberOfGenericArgumentsWithSuperArguments() - type.eclass->numberOfOwnGenericArguments();
                    for (size_t i = 0; i < type.eclass->numberOfOwnGenericArguments(); i++) {
                        if(!type.eclass->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments[i], type) ||
                           !type.genericArguments[i].compatibleTo(type.eclass->genericArgumentConstraints()[offset + i], type)) {
                            compilerError(token, "Dynamic casts involving generic type arguments are not possible yet. Please specify the generic argument constraints of the class for compatibility with future versions.");
                        }
                    }
                    
                    placeholder.write(originalType.type() == TT_SOMETHING || originalType.optional ? 0x44 : 0x40);
                    writer.writeCoin(type.eclass->index);
                    break;
                }
                case TT_PROTOCOL:
                    placeholder.write(originalType.type() == TT_SOMETHING || originalType.optional ? 0x45 : 0x41);
                    writer.writeCoin(type.protocol->index);
                    break;
                case TT_BOOLEAN:
                    placeholder.write(0x42);
                    break;
                case TT_INTEGER:
                    placeholder.write(0x43);
                    break;
                case TT_SYMBOL:
                    placeholder.write(0x46);
                    break;
                case TT_DOUBLE:
                    placeholder.write(0x47);
                    break;
                default: {
                    auto typeString = type.toString(typeContext, true);
                    compilerError(token, "You cannot cast to %s.", typeString.c_str());
                }
            }
            
            type.optional = true;
            return type;
        }
        case E_BEER_MUG: {
            writer.writeCoin(0x3A);
            
            Type t = parse(consumeToken(), token);
            
            if (!t.optional) {
                compilerError(token, "🍺 can only be used with optionals.");
            }
            
            t.optional = false;
            
            return t;
        }
        case E_CLINKING_BEER_MUGS: {
            writer.writeCoin(0x3B);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            const Token *methodToken = consumeToken();
            
            Type type = parse(consumeToken(), token);
            if (!type.optional) {
                compilerError(token, "🍻 may only be used on 🍬.");
            }
            
            auto method = type.eclass->getMethod(methodToken, type, typeContext);
            method->deprecatedWarning(methodToken);
            
            writer.writeCoin(method->vti);
            
            auto genericArgs = checkGenericArguments(method, token);
            auto typeContext = TypeContext(type, method, &genericArgs);
            
            checkAccess(method, token, "method");
            checkArguments(method->arguments, typeContext, token);
            
            placeholder.write();
            
            Type returnType = method->returnType;
            returnType.optional = true;
            return returnType.resolveOn(typeContext);
        }
        case E_DOUGHNUT: {
            writer.writeCoin(0x2);
            
            const Token *methodToken = consumeToken(IDENTIFIER);
            
            Type type = Type::parseAndFetchType(typeContext, intelligentDynamismLevel(), package, nullptr);
            
            if (type.optional) {
                compilerWarning(token, "Please remove useless 🍬.");
            }
            if (type.type() != TT_CLASS) {
                compilerError(token, "The given type is not a class.");
            }
            
            writer.writeCoin(type.eclass->index);
            
            auto method = type.eclass->getClassMethod(methodToken, type, typeContext);
            method->deprecatedWarning(methodToken);
            
            writer.writeCoin(method->vti);
            
            auto genericArgs = checkGenericArguments(method, token);
            auto typeContext = TypeContext(type, method, &genericArgs);
            
            checkAccess(method, token, "Class method");
            checkArguments(method->arguments, typeContext, token);
            
            return method->returnType.resolveOn(typeContext);
        }
        case E_HOT_PEPPER: {
            const Token *methodName = consumeToken();
            
            writer.writeCoin(0x71);
            
            Type type = parse(consumeToken(), token);
            
            if (type.type() != TT_CLASS) {
                compilerError(token, "You can only capture method calls on class instances.");
            }
            
            auto method = type.eclass->getMethod(methodName, type, typeContext);
            method->deprecatedWarning(methodName);
            
            writer.writeCoin(method->vti);
            
            return method->type();
        }
        case E_GRAPES: {
            writer.writeCoin(0x70);
            
            auto function = Closure(token);
            function.parseArgumentList(typeContext, package);
            function.parseReturnType(typeContext, package);
            
            auto variableCountPlaceholder = writer.writeCoinPlaceholder();
            auto coinCountPlaceholder = writer.writeCoinsCountPlaceholderCoin();
            
            Scope *closingScope = scoper.currentScope();
            if (!inClassContext) {
                scoper.pushScope(scoper.topScope());  // The object scope
            }
            
            function.firstToken = currentToken;
            
            auto sca = StaticFunctionAnalyzer(function, package, nullptr, inClassContext, typeContext, writer, scoper);
            sca.analyze(true, closingScope);
            
            if (!inClassContext) {
                scoper.popScope();
            }
            
            coinCountPlaceholder.write();
            variableCountPlaceholder.write(sca.localVariableCount());
            writer.writeCoin((EmojicodeCoin)function.arguments.size() | (sca.usedSelfInBody() ? 1 << 16 : 0));
            writer.writeCoin(variableCount);
            
            return function.type();
        }
        case E_LOLLIPOP: {
            writer.writeCoin(0x72);
            
            Type type = parse(consumeToken(), token);
            
            if (type.type() != TT_CALLABLE) {
                compilerError(token, "Given value is not callable.");
            }
            
            for (int i = 1; i <= type.arguments; i++) {
                parse(consumeToken(), token, type.genericArguments[i]);
            }
            
            return type.genericArguments[0];
        }
        case E_CHIPMUNK: {
            const Token *nameToken = consumeToken();
            
            if (inClassContext) {
                compilerError(token, "Not within an object-context.");
            }
            
            Class *superclass = typeContext.normalType.eclass->superclass;
            
            if (superclass == nullptr) {
                compilerError(token, "Class has no superclass.");
            }
            
            Method *method = superclass->getMethod(nameToken, superclass, typeContext);
            
            writer.writeCoin(0x5);
            writer.writeCoin(superclass->index);
            writer.writeCoin(method->vti);
            
            auto genericArgs = checkGenericArguments(method, token);
            auto tc = TypeContext(typeContext.normalType, method, &genericArgs);
            
            checkArguments(method->arguments, tc, token);
            
            return method->returnType.resolveOn(tc);
        }
        default: {
            auto placeholder = writer.writeCoinPlaceholder();
            
            const Token *tobject = consumeToken();
            
            Type type = parse(tobject, token).typeConstraintForReference(typeContext);
            
            if (type.optional) {
                compilerError(tobject, "You cannot call methods on optionals.");
            }
            
            Method *method;
            if (type.type() == TT_PROTOCOL) {
                method = type.protocol->getMethod(token, type, typeContext);
            }
            else if (type.type() == TT_CLASS) {
                method = type.eclass->getMethod(token, type, typeContext);
            }
            else {
                if (type.type() == TT_BOOLEAN) {
                    switch (token->value[0]) {
                        case E_NEGATIVE_SQUARED_CROSS_MARK:
                            placeholder.write(0x26);
                            return typeBoolean;
                        case E_PARTY_POPPER:
                            placeholder.write(0x27);
                            parse(consumeToken(), token, typeBoolean);
                            return typeBoolean;
                        case E_CONFETTI_BALL:
                            placeholder.write(0x28);
                            parse(consumeToken(), token, typeBoolean);
                            return typeBoolean;
                    }
                }
                else if (type.type() == TT_INTEGER) {
                    switch (token->value[0]) {
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(0x21);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(0x22);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(0x24);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(0x23);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(0x29);
                            parse(consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(0x2A);
                            parse(consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(0x2B);
                            parse(consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(0x2C);
                            parse(consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(0x25);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_LARGE_CIRCLE:
                            placeholder.write(0x5A);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_ANGER_SYMBOL:
                            placeholder.write(0x5B);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_CROSS_MARK:
                            placeholder.write(0x5C);
                            parse(consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_NO_ENTRY_SIGN:
                            placeholder.write(0x5D);
                            return typeInteger;
                        case E_ROCKET:
                            placeholder.write(0x3F);
                            return typeFloat;
                    }
                }
                else if (type.type() == TT_DOUBLE) {
                    switch (token->value[0]) {
                        case E_FACE_WITH_STUCK_OUT_TONGUE:
                            placeholder.write(0x2F);
                            parse(consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(0x30);
                            parse(consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(0x31);
                            parse(consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(0x33);
                            parse(consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(0x32);
                            parse(consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(0x34);
                            parse(consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(0x35);
                            parse(consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(0x36);
                            parse(consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(0x37);
                            parse(consumeToken(), token, typeFloat);
                            return typeBoolean;
                    }
                }
                
                if (token->value[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
                    parse(consumeToken(), token, type);  // Must be of the same type as the callee
                    placeholder.write(0x20);
                    return typeBoolean;
                }
                
                ecCharToCharStack(token->value[0], method);
                auto typeString = type.toString(typeContext, true);
                compilerError(token, "Unknown primitive method %s for %s.", method, typeString.c_str());
            }
            
            if (type.type() == TT_PROTOCOL) {
                placeholder.write(0x3);
                writer.writeCoin(type.protocol->index);
                writer.writeCoin(method->vti);
            }
            else if (type.type() == TT_CLASS) {
                placeholder.write(0x1);
                writer.writeCoin(method->vti);
            }
            
            method->deprecatedWarning(token);
            
            auto genericArgs = checkGenericArguments(method, token);
            auto typeContext = TypeContext(type, method, &genericArgs);
            
            checkAccess(method, token, "Method");
            checkArguments(method->arguments, typeContext, token);
            
            return method->returnType.resolveOn(typeContext);
        }
    }
    return typeNothingness;
}

StaticFunctionAnalyzer::StaticFunctionAnalyzer(Callable &callable, Package *p, Initializer *i, bool inClassContext,
                                               TypeContext typeContext, Writer &writer, Scoper &scoper)
        : callable(callable),
          writer(writer),
          scoper(scoper),
          initializer(i),
          inClassContext(inClassContext),
          typeContext(typeContext),
          package(p) {}

void StaticFunctionAnalyzer::analyze(bool compileDeadCode, Scope *copyScope) {
    currentToken = callable.firstToken;
    
    if (initializer) {
        scoper.currentScope()->changeInitializedBy(-1);
    }
    
    // Set the arguments to the method scope
    Scope methodScope(false);
    for (auto variable : callable.arguments) {
        uint8_t id = nextVariableID();
        CompilerVariable *varo = new CompilerVariable(variable.type, id, true, true, callable.dToken);
        
        methodScope.setLocalVariable(variable.name, varo);
    }
    
    if (copyScope) {
        variableCount += methodScope.copyFromScope(copyScope, nextVariableID());
    }
    
    scoper.pushScope(&methodScope);
    
    bool emittedDeadCodeWarning = false;
    
    const Token *token;
    while (token = consumeToken(), !(token->type == IDENTIFIER && token->value[0] == E_WATERMELON)) {
        effect = false;
        
        parse(token, callable.dToken);
        
        noEffectWarning(token);
       
        if (!emittedDeadCodeWarning && returned && nextToken()->value[0] != E_WATERMELON && token->type == IDENTIFIER) {
            compilerWarning(consumeToken(), "Dead code.");
            emittedDeadCodeWarning = true;
            if (!compileDeadCode) {
                break;
            }
        }
    }
    
    scoper.currentScope()->recommendFrozenVariables();
    scoper.popScope();
    noReturnError(callable.dToken);
    
    if (initializer) {
        scoper.currentScope()->initializerUnintializedVariablesCheck(initializer->dToken,
                                                                     "Instance variable \"%s\" must be initialized.");
        
        if (!calledSuper && typeContext.normalType.eclass->superclass) {
            ecCharToCharStack(initializer->name, initializerName);
            compilerError(initializer->dToken, "Missing call to superinitializer in initializer %s.", initializerName);
        }
    }
}

void StaticFunctionAnalyzer::writeAndAnalyzeProcedure(Procedure *procedure, Writer &writer, Type classType,
                                                      Scoper &scoper, bool inClassContext, Initializer *i) {
    writer.resetWrittenCoins();
    
    writer.writeEmojicodeChar(procedure->name);
    writer.writeUInt16(procedure->vti);
    writer.writeByte((uint8_t)procedure->arguments.size());
    
    if (procedure->native) {
        writer.writeByte(1);
        return;
    }
    writer.writeByte(0);
    
    auto variableCountPlaceholder = writer.writePlaceholder<unsigned char>();
    auto coinsCountPlaceholder = writer.writeCoinsCountPlaceholderCoin();
    
    auto sca = StaticFunctionAnalyzer(*procedure, procedure->package, i, inClassContext,
                                      TypeContext(classType, procedure), writer, scoper);
    sca.analyze();
    
    variableCountPlaceholder.write(sca.localVariableCount());
    coinsCountPlaceholder.write();
}