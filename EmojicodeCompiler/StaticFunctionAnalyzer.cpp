//
//  StaticFunctionAnalyzer.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "StaticFunctionAnalyzer.hpp"
#include "Type.hpp"
#include "utf8.h"
#include "Class.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "Procedure.hpp"
#include "CommonTypeFinder.hpp"
#include "VariableNotFoundErrorException.hpp"
#include "StringPool.hpp"

Type StaticFunctionAnalyzer::parse(const Token &token, const Token &parentToken,
                                   Type type, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = ctargs ? parse(token) : parse(token, type);
    if (!returnType.compatibleTo(type, typeContext, ctargs)) {
        auto cn = returnType.toString(typeContext, true);
        auto tn = type.toString(typeContext, true);
        throw CompilerErrorException(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

void StaticFunctionAnalyzer::noReturnError(SourcePosition p) {
    if (callable.returnType.type() != TT_NOTHINGNESS && !returned) {
        throw CompilerErrorException(p, "An explicit return is missing.");
    }
}

void StaticFunctionAnalyzer::noEffectWarning(const Token &warningToken) {
    if (!effect) {
        compilerWarning(warningToken, "Statement seems to have no effect whatsoever.");
    }
}

bool StaticFunctionAnalyzer::typeIsEnumerable(Type type, Type *elementType) {
    if (type.type() == TT_CLASS && !type.optional()) {
        for (Class *a = type.eclass; a != nullptr; a = a->superclass) {
            for (auto protocol : a->protocols()) {
                if (protocol.protocol == PR_ENUMERATEABLE) {
                    *elementType = protocol.resolveOn(type).genericArguments[0];
                    return true;
                }
            }
        }
    }
    return false;
}

Type StaticFunctionAnalyzer::parseProcedureCall(Type type, Procedure *p, const Token &token) {
    std::vector<Type> genericArguments;
    std::vector<CommonTypeFinder> genericArgsFinders;
    std::vector<Type> givenArgumentTypes;
    
    p->deprecatedWarning(token);
    
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken(IDENTIFIER);
        
        auto type = parseAndFetchType(typeContext, AllKindsOfDynamism);
        genericArguments.push_back(type);
    }
    
    auto inferGenericArguments = genericArguments.size() == 0 && p->genericArgumentVariables.size() > 0;
    auto typeContext = TypeContext(type, p, inferGenericArguments ? nullptr : &genericArguments);
    
    if (inferGenericArguments) {
        genericArgsFinders.resize(p->genericArgumentVariables.size());
    }
    else if (genericArguments.size() != p->genericArgumentVariables.size()) {
        throw CompilerErrorException(token, "Too few generic arguments provided.");
    }
    
    bool brackets = false;
    if (stream_.nextTokenIs(ARGUMENT_BRACKET_OPEN)) {
        stream_.consumeToken(ARGUMENT_BRACKET_OPEN);
        brackets = true;
    }
    for (auto var : p->arguments) {
        if (inferGenericArguments) {
            givenArgumentTypes.push_back(parse(stream_.consumeToken(), token, var.type.resolveOn(typeContext),
                                               &genericArgsFinders));
        }
        else {
            parse(stream_.consumeToken(), token, var.type.resolveOn(typeContext));
        }
    }
    if (brackets) {
        stream_.consumeToken(ARGUMENT_BRACKET_CLOSE);
    }
    
    if (inferGenericArguments) {
        for (size_t i = 0; i < genericArgsFinders.size(); i++) {
            auto commonType = genericArgsFinders[i].getCommonType(token);
            genericArguments.push_back(commonType);
            if (!commonType.compatibleTo(p->genericArgumentConstraints[i], typeContext)) {
                throw CompilerErrorException(token,
                                        "Infered type %s for generic argument %d is not compatible to constraint %s.",
                                             commonType.toString(typeContext, true).c_str(), i + 1,
                                             p->genericArgumentConstraints[i].toString(typeContext, true).c_str());
            }
        }
        typeContext = TypeContext(type, p, &genericArguments);
        for (size_t i = 0; i < givenArgumentTypes.size(); i++) {
            auto givenTypen = givenArgumentTypes[i];
            auto expectedType = p->arguments[i].type.resolveOn(typeContext);
            if (!givenTypen.compatibleTo(expectedType, typeContext)) {
                auto cn = expectedType.toString(typeContext, true);
                auto tn = givenTypen.toString(typeContext, true);
                throw CompilerErrorException(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
            }
        }
    }
    
    if (p->access == PRIVATE) {
        if (typeContext.calleeType().type() != TT_CLASS || p->eclass != typeContext.calleeType().eclass) {
            ecCharToCharStack(p->name, nm);
            throw CompilerErrorException(token, "%s is 🔒.", nm);
        }
    }
    else if (p->access == PROTECTED) {
        if (typeContext.calleeType().type() != TT_CLASS || !typeContext.calleeType().eclass->inheritsFrom(p->eclass)) {
            ecCharToCharStack(p->name, nm);
            throw CompilerErrorException(token, "%s is 🔐.", nm);
        }
    }
    
    return p->returnType.resolveOn(typeContext);
}

void StaticFunctionAnalyzer::writeCoinForScopesUp(bool inObjectScope, EmojicodeCoin stack,
                                                  EmojicodeCoin object, SourcePosition p) {
    if (!inObjectScope) {
        writer.writeCoin(stack, p);
    }
    else {
        writer.writeCoin(object, p);
        usedSelf = true;
    }
}

void StaticFunctionAnalyzer::flowControlBlock() {
    scoper.currentScope().changeInitializedBy(1);
    if (!inClassContext) {
        scoper.objectScope()->changeInitializedBy(1);
    }
    
    flowControlDepth++;
    
    auto &token = stream_.consumeToken(IDENTIFIER);
    if (token.value[0] != E_GRAPES) {
        ecCharToCharStack(token.value[0], s);
        throw CompilerErrorException(token, "Expected 🍇 but found %s instead.", s);
    }
    
    auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        effect = false;
        auto &token = stream_.consumeToken();
        parse(token);
        noEffectWarning(token);
    }
    stream_.consumeToken();
    placeholder.write();
    
    effect = true;
    
    scoper.currentScope().changeInitializedBy(-1);
    if (!inClassContext) {
        scoper.objectScope()->changeInitializedBy(-1);
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

void StaticFunctionAnalyzer::parseIfExpression(const Token &token) {
    if (stream_.nextTokenIs(E_SOFT_ICE_CREAM)) {
        stream_.consumeToken(IDENTIFIER);
        writer.writeCoin(0x3E, token);
        
        auto &varName = stream_.consumeToken(VARIABLE);
        if (scoper.currentScope().hasLocalVariable(varName.value)) {
            throw CompilerErrorException(token, "Cannot redeclare variable.");
        }
        
        int id = scoper.reserveVariableSlot();
        writer.writeCoin(id, token);
        
        Type t = parse(stream_.consumeToken());
        if (!t.optional()) {
            throw CompilerErrorException(token, "🍊🍦 can only be used with optionals.");
        }
        
        t = t.copyWithoutOptional();
        
        scoper.currentScope().setLocalVariable(varName.value, Variable(t, id, 1, true, varName));
    }
    else {
        parse(stream_.consumeToken(NO_TYPE), token, typeBoolean);
    }
}

Type StaticFunctionAnalyzer::parse(const Token &token, Type expectation) {
    switch (token.type()) {
        case STRING: {
            writer.writeCoin(0x10, token);
            writer.writeCoin(StringPool::theStringPool().poolString(token.value), token);
            return Type(CL_STRING);
        }
        case BOOLEAN_TRUE:
            writer.writeCoin(0x11, token);
            return typeBoolean;
        case BOOLEAN_FALSE:
            writer.writeCoin(0x12, token);
            return typeBoolean;
        case INTEGER: {
            /* We know token->value only contains ints less than 255 */
            const char *string = token.value.utf8CString();
            
            EmojicodeInteger l = strtoll(string, nullptr, 0);
            delete [] string;
            
            if (expectation.type() == TT_DOUBLE) {
                writer.writeCoin(0x15, token);
                writer.writeDoubleCoin(l, token);
                return typeFloat;
            }
            
            if (llabs(l) > INT32_MAX) {
                writer.writeCoin(0x14, token);
                
                writer.writeCoin(l >> 32, token);
                writer.writeCoin((EmojicodeCoin)l, token);
                
                return typeInteger;
            }
            else {
                writer.writeCoin(0x13, token);
                writer.writeCoin((EmojicodeCoin)l, token);
                
                return typeInteger;
            }
        }
        case DOUBLE: {
            writer.writeCoin(0x15, token);
            
            const char *string = token.value.utf8CString();
            
            double d = strtod(string, nullptr);
            delete [] string;
            writer.writeDoubleCoin(d, token);
            return typeFloat;
        }
        case SYMBOL:
            writer.writeCoin(0x16, token);
            writer.writeCoin(token.value[0], token);
            return typeSymbol;
        case VARIABLE: {
            auto var = scoper.getVariable(token.value, token.position());
            
            var.first.uninitalizedError(token);
            
            writeCoinForScopesUp(var.second, 0x1A, 0x1C, token);
            writer.writeCoin(var.first.id, token);
            
            return var.first.type;
        }
        case IDENTIFIER:
            return parseIdentifier(token, expectation);
        case DOCUMENTATION_COMMENT:
            throw CompilerErrorException(token, "Misplaced documentation comment.");
        case ARGUMENT_BRACKET_OPEN:
            throw CompilerErrorException(token, "Unexpected 〖");
        case ARGUMENT_BRACKET_CLOSE:
            throw CompilerErrorException(token, "Unexpected 〗");
        case NO_TYPE:
        case COMMENT:
            break;
    }
    throw CompilerErrorException(token, "Cannot determine expression’s return type.");
}

Type StaticFunctionAnalyzer::parseIdentifier(const Token &token, Type expectation) {
    if (token.value[0] != E_RED_APPLE) {
        // We need a chance to test whether the red apple’s return is used
        effect = true;
    }

    switch (token.value[0]) {
        case E_SHORTCAKE: {
            auto &varName = stream_.consumeToken(VARIABLE);
            
            if (scoper.currentScope().hasLocalVariable(varName.value)) {
                throw CompilerErrorException(token, "Cannot redeclare variable.");
            }
            
            Type t = parseAndFetchType(typeContext, AllKindsOfDynamism);
            
            int id = scoper.reserveVariableSlot();
            scoper.currentScope().setLocalVariable(varName.value,
                                                   Variable(t, id, t.optional() ? 1 : 0, false, varName));
            if (t.optional()) {
                writer.writeCoin(0x1B, token);
                writer.writeCoin(id, token);
                writer.writeCoin(0x17, token);
            }
            return typeNothingness;
        }
        case E_CUSTARD: {
            auto &varName = stream_.consumeToken(VARIABLE);
            
            Type type = typeNothingness;
            try {
                auto var = scoper.getVariable(varName.value, varName.position());
                if (var.first.initialized <= 0) {
                    var.first.initialized = 1;
                }
                
                var.first.mutate(varName);
                
                writeCoinForScopesUp(var.second, 0x1B, 0x1D, token);
                writer.writeCoin(var.first.id, token);
                
                type = var.first.type;
            }
            catch (VariableNotFoundErrorException &vne) {
                // Not declared, declaring as local variable
                writer.writeCoin(0x1B, token);
                
                int id = scoper.reserveVariableSlot();
                writer.writeCoin(id, token);
                
                Type t = parse(stream_.consumeToken());
                scoper.currentScope().setLocalVariable(varName.value, Variable(t, id, 1, false, varName));
                return typeNothingness;
            }

            parse(stream_.consumeToken(), token, type);
            
            return typeNothingness;
        }
        case E_SOFT_ICE_CREAM: {
            auto &varName = stream_.consumeToken(VARIABLE);
            
            if (scoper.currentScope().hasLocalVariable(varName.value)) {
                throw CompilerErrorException(token, "Cannot redeclare variable.");
            }
            
            writer.writeCoin(0x1B, token);
            
            int id = scoper.reserveVariableSlot();
            writer.writeCoin(id, token);
            
            Type t = parse(stream_.consumeToken());
            scoper.currentScope().setLocalVariable(varName.value, Variable(t, id, 1, true, varName));
            return typeNothingness;
        }
        case E_COOKING:
        case E_CHOCOLATE_BAR: {
            auto &varName = stream_.consumeToken(VARIABLE);
            
            auto var = scoper.getVariable(varName.value, varName.position());
            
            var.first.uninitalizedError(varName);
            var.first.mutate(varName);
            
            if (!var.first.type.compatibleTo(typeInteger, typeContext)) {
                ecCharToCharStack(token.value[0], ls);
                throw CompilerErrorException(token, "%s can only operate on 🚂 variables.", ls);
            }
            
            if (token.value[0] == E_COOKING) {
                writeCoinForScopesUp(var.second, 0x19, 0x1F, token);
            }
            else {
                writeCoinForScopesUp(var.second, 0x18, 0x1E, token);
            }
            
            writer.writeCoin(var.first.id, token);
            
            return typeNothingness;
        }
        case E_COOKIE: {
            writer.writeCoin(0x52, token);
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            int stringCount = 0;
            
            while (stream_.nextTokenIsEverythingBut(E_COOKIE)) {
                parse(stream_.consumeToken(), token, Type(CL_STRING));
                stringCount++;
            }
            stream_.consumeToken(IDENTIFIER);
            
            if (stringCount == 0) {
                throw CompilerErrorException(token, "An empty 🍪 is invalid.");
            }
            
            placeholder.write(stringCount);
            return Type(CL_STRING);
        }
        case E_ICE_CREAM: {
            writer.writeCoin(0x51, token);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            
            Type type = Type(CL_LIST);
            if (expectation.type() == TT_CLASS && expectation.eclass == CL_LIST) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, listType);
                }
                stream_.consumeToken(IDENTIFIER);
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    ct.addType(parse(stream_.consumeToken()), typeContext);
                }
                stream_.consumeToken(IDENTIFIER);
                type.genericArguments[0] = ct.getCommonType(token);
            }
            
            placeholder.write();
            return type;
        }
        case E_HONEY_POT: {
            writer.writeCoin(0x50, token);
           
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            Type type = Type(CL_DICTIONARY);
            
            if (expectation.type() == TT_CLASS && expectation.eclass == CL_DICTIONARY) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, Type(CL_STRING));
                    parse(stream_.consumeToken(), token, listType);
                }
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, Type(CL_STRING));
                    ct.addType(parse(stream_.consumeToken()), typeContext);
                }
                stream_.consumeToken(IDENTIFIER);
                type.genericArguments[0] = ct.getCommonType(token);
            }
            
            placeholder.write();
            return type;
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE: {
            writer.writeCoin(0x53, token);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            writer.writeCoin(0x54, token);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_TANGERINE: {
            writer.writeCoin(0x62, token);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            auto fcr = FlowControlReturn();
            
            parseIfExpression(token);
            
            flowControlBlock();
            flowControlReturnEnd(fcr);
            
            while (stream_.nextTokenIs(E_LEMON)) {
                stream_.consumeToken();
                writer.writeCoin(0x63, token);
                
                parseIfExpression(token);
                flowControlBlock();
                flowControlReturnEnd(fcr);
            }
            
            if (stream_.nextTokenIs(E_STRAWBERRY)) {
                stream_.consumeToken();
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
            writer.writeCoin(0x61, token);
            
            parse(stream_.consumeToken(), token, typeBoolean);
            flowControlBlock();
            returned = false;
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            auto &variableToken = stream_.consumeToken(VARIABLE);
            
            if (scoper.currentScope().hasLocalVariable(variableToken.value)) {
                throw CompilerErrorException(variableToken, "Cannot redeclare variable.");
            }
            
            int vID = scoper.reserveVariableSlot();
            writer.writeCoin(vID, token);
            
            Type iteratee = parse(stream_.consumeToken(), token, typeSomeobject);
            
            Type itemType = typeNothingness;
            
            if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_LIST) {
                // If the iteratee is a list, the Real-Time Engine has some special sugar
                placeholder.write(0x65);
                writer.writeCoin(scoper.reserveVariableSlot(), token);  //Internally needed
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(iteratee.genericArguments[0], vID, true, true, variableToken));
            }
            else if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_RANGE) {
                // If the iteratee is a range, the Real-Time Engine also has some special sugar
                placeholder.write(0x66);
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(typeInteger, vID, true, true, variableToken));
            }
            else if (typeIsEnumerable(iteratee, &itemType)) {
                placeholder.write(0x64);
                writer.writeCoin(scoper.reserveVariableSlot(), token);  //Internally needed
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(itemType, vID, true, true, variableToken));
            }
            else {
                auto iterateeString = iteratee.toString(typeContext, true);
                throw CompilerErrorException(token, "%s does not conform to s🔂.", iterateeString.c_str());
            }
            
            flowControlBlock();
            returned = false;
            
            return typeNothingness;
        }
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_ROOTSTER: {
            ecCharToCharStack(token.value[0], identifier);
            throw CompilerErrorException(token, "Unexpected identifier %s.", identifier);
            return typeNothingness;
        }
        case E_DOG: {
            usedSelf = true;
            writer.writeCoin(0x3C, token);
            if (initializer && !calledSuper && initializer->eclass->superclass) {
                throw CompilerErrorException(token, "Attempt to use 🐕 before superinitializer call.");
            }
            
            if (inClassContext) {
                throw CompilerErrorException(token, "Illegal use of 🐕.");
                break;
            }
            
            scoper.objectScope()->initializerUnintializedVariablesCheck(token,
                                                "Instance variable \"%s\" must be initialized before the use of 🐕.");
            
            return typeContext.calleeType();
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writer.writeCoin(0x17, token);
            return typeNothingness;
        }
        case E_CLOUD: {
            writer.writeCoin(0x2E, token);
            parse(stream_.consumeToken());
            return typeBoolean;
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writer.writeCoin(0x2D, token);
            
            parse(stream_.consumeToken(), token, typeSomeobject);
            parse(stream_.consumeToken(), token, typeSomeobject);
            
            return typeBoolean;
        }
        case E_GOAT: {
            if (!initializer) {
                throw CompilerErrorException(token, "🐐 can only be used inside initializers.");
                break;
            }
            if (!typeContext.calleeType().eclass->superclass) {
                throw CompilerErrorException(token, "🐐 can only be used if the eclass inherits from another.");
                break;
            }
            if (calledSuper) {
                throw CompilerErrorException(token, "You may not call more than one superinitializer.");
            }
            if (flowControlDepth) {
                throw CompilerErrorException(token, "You may not put a call to a superinitializer in a flow control structure.");
            }
            
            scoper.objectScope()->initializerUnintializedVariablesCheck(token,
                                            "Instance variable \"%s\" must be initialized before superinitializer.");
            
            writer.writeCoin(0x3D, token);
            
            Class *eclass = typeContext.calleeType().eclass;
            
            writer.writeCoin(eclass->superclass->index, token);
            
            auto &initializerToken = stream_.consumeToken(IDENTIFIER);
            
            auto initializer = eclass->superclass->getInitializer(initializerToken, Type(eclass), typeContext);
            
            writer.writeCoin(initializer->vti(), token);
            
            parseProcedureCall(typeContext.calleeType(), initializer, token);

            calledSuper = true;
            
            return typeNothingness;
        }
        case E_RED_APPLE: {
            if (effect) {
                // Effect is true, so apple is called as subcommand
                throw CompilerErrorException(token, "🍎’s return may not be used as an argument.");
            }
            effect = true;
            
            writer.writeCoin(0x60, token);
            
            if (initializer) {
                if (initializer->canReturnNothingness) {
                    parse(stream_.consumeToken(), token, typeNothingness);
                    return typeNothingness;
                }
                else {
                    throw CompilerErrorException(token, "🍎 cannot be used inside a initializer.");
                }
            }
            
            parse(stream_.consumeToken(), token, callable.returnType);
            returned = true;
            return typeNothingness;
        }
        case E_BLACK_SQUARE_BUTTON: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            Type originalType = parse(stream_.consumeToken(), token, typeSomething);
            Type type = parseAndFetchType(typeContext, NoDynamism, expectation);
            
            if (originalType.compatibleTo(type, typeContext)) {
                compilerWarning(token, "Superfluous cast.");
            }
            else if (!type.compatibleTo(originalType, typeContext)) {
                auto typeString = type.toString(typeContext, true);
                compilerWarning(token, "Cast to unrelated type %s will always fail.", typeString.c_str());
            }
            
            switch (type.type()) {
                case TT_CLASS: {
                    auto offset = type.eclass->numberOfGenericArgumentsWithSuperArguments() - type.eclass->numberOfOwnGenericArguments();
                    for (size_t i = 0; i < type.eclass->numberOfOwnGenericArguments(); i++) {
                        if(!type.eclass->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments[i], type) ||
                           !type.genericArguments[i].compatibleTo(type.eclass->genericArgumentConstraints()[offset + i], type)) {
                            throw CompilerErrorException(token, "Dynamic casts involving generic type arguments are not possible yet. Please specify the generic argument constraints of the class for compatibility with future versions.");
                        }
                    }
                    
                    placeholder.write(originalType.type() == TT_SOMETHING || originalType.optional() ? 0x44 : 0x40);
                    writer.writeCoin(type.eclass->index, token);
                    break;
                }
                case TT_PROTOCOL:
                    placeholder.write(originalType.type() == TT_SOMETHING || originalType.optional() ? 0x45 : 0x41);
                    writer.writeCoin(type.protocol->index, token);
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
                    throw CompilerErrorException(token, "You cannot cast to %s.", typeString.c_str());
                }
            }
            
            type.setOptional();
            return type;
        }
        case E_BEER_MUG: {
            writer.writeCoin(0x3A, token);
            
            Type t = parse(stream_.consumeToken());
            
            if (!t.optional()) {
                throw CompilerErrorException(token, "🍺 can only be used with optionals.");
            }
            
            return t.copyWithoutOptional();
        }
        case E_CLINKING_BEER_MUGS: {
            writer.writeCoin(0x3B, token);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            
            auto &methodToken = stream_.consumeToken();
            
            Type type = parse(stream_.consumeToken());
            if (!type.optional()) {
                throw CompilerErrorException(token, "🍻 may only be used on 🍬.");
            }
            
            auto method = type.eclass->getMethod(methodToken, type, typeContext);
            
            writer.writeCoin(method->vti(), token);
            
            parseProcedureCall(type, method, token);
            
            placeholder.write();
            
            Type returnType = method->returnType;
            returnType.setOptional();
            return returnType.resolveOn(typeContext);
        }
        case E_HOT_PEPPER: {
            auto &methodName = stream_.consumeToken();
            
            writer.writeCoin(0x71, token);
            
            Type type = parse(stream_.consumeToken());
            
            if (type.type() != TT_CLASS) {
                throw CompilerErrorException(token, "You can only capture method calls on class instances.");
            }
            
            auto method = type.eclass->getMethod(methodName, type, typeContext);
            method->deprecatedWarning(methodName);
            
            writer.writeCoin(method->vti(), token);
            
            return method->type();
        }
        case E_GRAPES: {
            writer.writeCoin(0x70, token);
            
            auto function = Closure(token.position());
            parseArgumentList(&function, typeContext);
            parseReturnType(&function, typeContext);
            parseBody(&function);
            
            auto variableCountPlaceholder = writer.writeCoinPlaceholder(token);
            auto coinCountPlaceholder = writer.writeCoinsCountPlaceholderCoin(token);
            
            auto flattenedResult = scoper.flattenedCopy(static_cast<int>(function.arguments.size()));
            auto closureScoper = flattenedResult.first;
            
            auto analyzer = StaticFunctionAnalyzer(function, package_, nullptr, inClassContext, typeContext,
                                                   writer, closureScoper);
            analyzer.analyze(true);
            
            coinCountPlaceholder.write();
            variableCountPlaceholder.write(closureScoper.numberOfReservations());
            writer.writeCoin(static_cast<EmojicodeCoin>(function.arguments.size())
                             | (analyzer.usedSelfInBody() ? 1 << 16 : 0), token);
            writer.writeCoin(flattenedResult.second, token);
            
            return function.type();
        }
        case E_LOLLIPOP: {
            writer.writeCoin(0x72, token);
            
            Type type = parse(stream_.consumeToken());
            
            if (type.type() != TT_CALLABLE) {
                throw CompilerErrorException(token, "Given value is not callable.");
            }
            
            for (int i = 1; i <= type.arguments; i++) {
                parse(stream_.consumeToken(), token, type.genericArguments[i]);
            }
            
            return type.genericArguments[0];
        }
        case E_CHIPMUNK: {
            auto &nameToken = stream_.consumeToken();
            
            if (inClassContext) {
                throw CompilerErrorException(token, "Not within an object-context.");
            }
            
            Class *superclass = typeContext.calleeType().eclass->superclass;
            
            if (superclass == nullptr) {
                throw CompilerErrorException(token, "Class has no superclass.");
            }
            
            Method *method = superclass->getMethod(nameToken, Type(superclass), typeContext);
            
            writer.writeCoin(0x5, token);
            writer.writeCoin(superclass->index, token);
            writer.writeCoin(method->vti(), token);
            
            return parseProcedureCall(typeContext.calleeType(), method, token);
        }
        case E_LARGE_BLUE_DIAMOND: {
            TypeDynamism dynamism;
            Type type = parseAndFetchType(typeContext, AllKindsOfDynamism, expectation, &dynamism)
                        .resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (type.optional()) {
                throw CompilerErrorException(token, "Optionals cannot be instantiated.");
            }
            
            if (type.type() == TT_ENUM) {
                if (dynamism != NoDynamism) {
                    throw CompilerErrorException(token, "Enums cannot be instantiated dynamically.");
                }
                
                writer.writeCoin(0x13, token);
                
                auto name = stream_.consumeToken(IDENTIFIER);
                
                auto v = type.eenum->getValueFor(name.value[0]);
                if (!v.first) {
                    ecCharToCharStack(name.value[0], valueName);
                    ecCharToCharStack(type.eenum->name(), enumName);
                    throw CompilerErrorException(name, "%s does not have a member named %s.", enumName, valueName);
                }
                else if (v.second > UINT32_MAX) {
                    writer.writeCoin(v.second >> 32, token);
                    writer.writeCoin((EmojicodeCoin)v.second, token);
                }
                else {
                    writer.writeCoin((EmojicodeCoin)v.second, token);
                }
                
                return type;
            }
            else if (type.type() == TT_CLASS) {
                writer.writeCoin(0x4, token);
                
                if (dynamism) {
                    writer.writeCoin(UINT32_MAX, token);
                }
                else {
                    writer.writeCoin(type.eclass->index, token);
                }
                
                auto initializerName = stream_.consumeToken(IDENTIFIER);
                
                Initializer *initializer = type.eclass->getInitializer(initializerName, type, typeContext);
                
                if (dynamism == Self && !initializer->required) {
                    throw CompilerErrorException(initializerName,
                                                 "Only required initializers can be used with dynamic types.");
                }
                else if (dynamism == GenericTypeVariables) {
                    throw CompilerErrorException(initializerName, "You cannot instantiate generic types yet.");
                }
                
                initializer->deprecatedWarning(initializerName);
                
                writer.writeCoin(initializer->vti(), token);
                
                parseProcedureCall(type, initializer, token);
                
                if (initializer->canReturnNothingness) {
                    type.setOptional();
                }
                return type;
            }
            else {
                throw CompilerErrorException(token, "The given type cannot be instantiated.");
            }
        }
        case E_DOUGHNUT: {
            writer.writeCoin(0x2, token);
            
            auto &methodToken = stream_.consumeToken(IDENTIFIER);
            
            TypeDynamism dynamism;
            Type type = parseAndFetchType(typeContext, AllKindsOfDynamism, expectation, &dynamism)
                        .resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (type.optional()) {
                compilerWarning(token, "Please remove useless 🍬.");
            }
            if (type.type() != TT_CLASS) {
                throw CompilerErrorException(token, "The given type is not a class.");
            }
            if (dynamism == GenericTypeVariables) {
                throw CompilerErrorException(token, "You cannot call methods generic types yet.");
            }
            
            writer.writeCoin(type.eclass->index, token);
            
            auto method = type.eclass->getClassMethod(methodToken, type, typeContext);
            
            writer.writeCoin(method->vti(), token);
            
            return parseProcedureCall(type, method, token);
        }
        default: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            auto &tobject = stream_.consumeToken();
            
            Type type = parse(tobject).resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (type.optional()) {
                throw CompilerErrorException(tobject, "You cannot call methods on optionals.");
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
                    switch (token.value[0]) {
                        case E_NEGATIVE_SQUARED_CROSS_MARK:
                            placeholder.write(0x26);
                            return typeBoolean;
                        case E_PARTY_POPPER:
                            placeholder.write(0x27);
                            parse(stream_.consumeToken(), token, typeBoolean);
                            return typeBoolean;
                        case E_CONFETTI_BALL:
                            placeholder.write(0x28);
                            parse(stream_.consumeToken(), token, typeBoolean);
                            return typeBoolean;
                    }
                }
                else if (type.type() == TT_INTEGER) {
                    switch (token.value[0]) {
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(0x21);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(0x22);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(0x24);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(0x23);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(0x29);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(0x2A);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(0x2B);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(0x2C);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeBoolean;
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(0x25);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_HEAVY_LARGE_CIRCLE:
                            placeholder.write(0x5A);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_ANGER_SYMBOL:
                            placeholder.write(0x5B);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_CROSS_MARK:
                            placeholder.write(0x5C);
                            parse(stream_.consumeToken(), token, typeInteger);
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
                    switch (token.value[0]) {
                        case E_FACE_WITH_STUCK_OUT_TONGUE:
                            placeholder.write(0x2F);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(0x30);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(0x31);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(0x33);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(0x32);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeFloat;
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(0x34);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(0x35);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(0x36);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(0x37);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeBoolean;
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(0x38);
                            parse(stream_.consumeToken(), token, typeFloat);
                            return typeFloat;
                    }
                }
                
                if (token.value[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
                    parse(stream_.consumeToken(), token, type);  // Must be of the same type as the callee
                    placeholder.write(0x20);
                    return typeBoolean;
                }
                
                ecCharToCharStack(token.value[0], method);
                auto typeString = type.toString(typeContext, true);
                throw CompilerErrorException(token, "Unknown primitive method %s for %s.", method, typeString.c_str());
            }
            
            if (type.type() == TT_PROTOCOL) {
                placeholder.write(0x3);
                writer.writeCoin(type.protocol->index, token);
                writer.writeCoin(method->vti(), token);
            }
            else if (type.type() == TT_CLASS) {
                placeholder.write(0x1);
                writer.writeCoin(method->vti(), token);
            }
            
            return parseProcedureCall(type, method, token);
        }
    }
    return typeNothingness;
}

StaticFunctionAnalyzer::StaticFunctionAnalyzer(Callable &callable, Package *p, Initializer *i, bool inClassContext,
                                               TypeContext typeContext, Writer &writer, CallableScoper &scoper)
        : AbstractParser(p, callable.tokenStream()),
          callable(callable),
          writer(writer),
          scoper(scoper),
          initializer(i),
          inClassContext(inClassContext),
          typeContext(typeContext) {}

void StaticFunctionAnalyzer::analyze(bool compileDeadCode) {
    if (initializer) {
        scoper.objectScope()->changeInitializedBy(-1);
    }
    try {
        Scope &methodScope = scoper.pushScope();
        for (size_t i = 0; i < callable.arguments.size(); i++) {
            auto variable = callable.arguments[i];
            methodScope.setLocalVariable(variable.name.value, Variable(variable.type, i, true, true, variable.name));
        }
        scoper.ensureNReservations(static_cast<int>(callable.arguments.size()));
        
        bool emittedDeadCodeWarning = false;
        
        while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
            effect = false;
            
            auto &token = stream_.consumeToken();
            parse(token);
            noEffectWarning(token);
           
            if (!emittedDeadCodeWarning && returned && !stream_.nextTokenIs(E_WATERMELON)) {
                compilerWarning(stream_.consumeToken(), "Dead code.");
                emittedDeadCodeWarning = true;
                if (!compileDeadCode) {
                    break;
                }
            }
        }
        
        scoper.popScopeAndRecommendFrozenVariables();
        noReturnError(callable.position());
        
        if (initializer) {
            scoper.objectScope()->initializerUnintializedVariablesCheck(initializer->position(),
                                                                    "Instance variable \"%s\" must be initialized.");
            
            if (!calledSuper && typeContext.calleeType().eclass->superclass) {
                ecCharToCharStack(initializer->name, initializerName);
                throw CompilerErrorException(initializer->position(),
                              "Missing call to superinitializer in initializer %s.", initializerName);
            }
        }
    }
    catch (CompilerErrorException &ce) {
        printError(ce);
    }
}

void StaticFunctionAnalyzer::writeAndAnalyzeProcedure(Procedure *procedure, Writer &writer, Type classType,
                                                      CallableScoper &scoper, bool inClassContext, Initializer *i) {
    writer.resetWrittenCoins();
    
    writer.writeEmojicodeChar(procedure->name);
    writer.writeUInt16(procedure->vti());
    writer.writeByte(static_cast<uint8_t>(procedure->arguments.size()));
    
    if (procedure->native) {
        writer.writeByte(1);
        return;
    }
    writer.writeByte(0);
    
    auto variableCountPlaceholder = writer.writePlaceholder<unsigned char>();
    auto coinsCountPlaceholder = writer.writeCoinsCountPlaceholderCoin(procedure->position());
    
    auto sca = StaticFunctionAnalyzer(*procedure, procedure->package, i, inClassContext,
                                      TypeContext(classType, procedure), writer, scoper);
    sca.analyze();
    
    variableCountPlaceholder.write(scoper.numberOfReservations());
    coinsCountPlaceholder.write();
}