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
#include "StringPool.hpp"

Type StaticFunctionAnalyzer::parse(const Token &token, const Token &parentToken, Type type) {
    auto returnType = parse(token);
    if (!returnType.compatibleTo(type, typeContext)) {
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

void StaticFunctionAnalyzer::checkAccess(Procedure *p, const Token &token, const char *type) {
    if (p->access == PRIVATE) {
        if (typeContext.normalType.type() != TT_CLASS || p->eclass != typeContext.normalType.eclass) {
            ecCharToCharStack(p->name, nm);
            throw CompilerErrorException(token, "%s %s is 🔒.", type, nm);
        }
    }
    else if (p->access == PROTECTED) {
        if (typeContext.normalType.type() != TT_CLASS || !typeContext.normalType.eclass->inheritsFrom(p->eclass)) {
            ecCharToCharStack(p->name, nm);
            throw CompilerErrorException(token, "%s %s is 🔐.", type, nm);
        }
    }
}

std::vector<Type> StaticFunctionAnalyzer::checkGenericArguments(Procedure *p, const Token &token) {
    std::vector<Type> k;
    
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken(IDENTIFIER);
        
        auto type = parseAndFetchType(typeContext, AllKindsOfDynamism);
        k.push_back(type);
    }
    
    if (k.size() != p->genericArgumentVariables.size()) {
        throw CompilerErrorException(token, "Too few generic arguments provided.");
    }
    
    return k;
}

void StaticFunctionAnalyzer::checkArguments(std::vector<Argument> arguments, TypeContext calledType,
                                            const Token &token) {
    bool brackets = false;
    if (stream_.nextTokenIs(ARGUMENT_BRACKET_OPEN)) {
        stream_.consumeToken(ARGUMENT_BRACKET_OPEN);
        brackets = true;
    }
    for (auto var : arguments) {
        parse(stream_.consumeToken(), token, var.type.resolveOn(calledType));
    }
    if (brackets) {
        stream_.consumeToken(ARGUMENT_BRACKET_CLOSE);
    }
}

void StaticFunctionAnalyzer::writeCoinForScopesUp(bool inObjectScope, EmojicodeCoin stack, EmojicodeCoin object) {
    if (!inObjectScope) {
        writer.writeCoin(stack);
    }
    else {
        writer.writeCoin(object);
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
    
    auto placeholder = writer.writeCoinsCountPlaceholderCoin();
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
        writer.writeCoin(0x3E);
        
        auto &varName = stream_.consumeToken(VARIABLE);
        if (scoper.currentScope().hasLocalVariable(varName.value)) {
            throw CompilerErrorException(token, "Cannot redeclare variable.");
        }
        
        int id = scoper.reserveVariableSlot();
        writer.writeCoin(id);
        
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

Type StaticFunctionAnalyzer::parse(const Token &token) {
    switch (token.type()) {
        case STRING: {
            writer.writeCoin(0x10);
            writer.writeCoin(StringPool::theStringPool().poolString(token.value));
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
            const char *string = token.value.utf8CString();
            
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
            
            const char *string = token.value.utf8CString();
            
            double d = strtod(string, nullptr);
            delete [] string;
            writer.writeDouble(d);
            return typeFloat;
        }
        case SYMBOL:
            writer.writeCoin(0x16);
            writer.writeCoin(token.value[0]);
            return typeSymbol;
        case VARIABLE: {
            auto var = scoper.getVariable(token.value, token.position());
            
            var.first.uninitalizedError(token);
            
            writeCoinForScopesUp(var.second, 0x1A, 0x1C);
            writer.writeCoin(var.first.id);
            
            return var.first.type;
        }
        case IDENTIFIER:
            return parseIdentifier(token);
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

Type StaticFunctionAnalyzer::parseIdentifier(const Token &token) {
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
                writer.writeCoin(0x1B);
                writer.writeCoin(id);
                writer.writeCoin(0x17);
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
                
                writeCoinForScopesUp(var.second, 0x1B, 0x1D);
                writer.writeCoin(var.first.id);
                
                type = var.first.type;
            }
            catch (CompilerErrorException &ce) {
                // Not declared, declaring as local variable
                writer.writeCoin(0x1B);
                
                int id = scoper.reserveVariableSlot();
                writer.writeCoin(id);
                
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
            
            writer.writeCoin(0x1B);
            
            int id = scoper.reserveVariableSlot();
            writer.writeCoin(id);
            
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
                writeCoinForScopesUp(var.second, 0x19, 0x1F);
            }
            else {
                writeCoinForScopesUp(var.second, 0x18, 0x1E);
            }
            
            writer.writeCoin(var.first.id);
            
            return typeNothingness;
        }
        case E_COOKIE: {
            writer.writeCoin(0x52);
            auto placeholder = writer.writeCoinPlaceholder();
            
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
            writer.writeCoin(0x51);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            CommonTypeFinder ct;
            
            while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                ct.addType(parse(stream_.consumeToken()), typeContext);
            }
            stream_.consumeToken(IDENTIFIER);
            
            placeholder.write();
            
            Type type = Type(CL_LIST);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_HONEY_POT: {
            writer.writeCoin(0x50);
           
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            CommonTypeFinder ct;
            
            while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                parse(stream_.consumeToken(), token, Type(CL_STRING));
                ct.addType(parse(stream_.consumeToken()), typeContext);
            }
            stream_.consumeToken(IDENTIFIER);
            
            placeholder.write();
            
            Type type = Type(CL_DICTIONARY);
            type.genericArguments[0] = ct.getCommonType(token);
            
            return type;
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE: {
            writer.writeCoin(0x53);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            writer.writeCoin(0x54);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            parse(stream_.consumeToken(), token, typeInteger);
            return Type(CL_RANGE);
        }
        case E_TANGERINE: {
            writer.writeCoin(0x62);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            auto fcr = FlowControlReturn();
            
            parseIfExpression(token);
            
            flowControlBlock();
            flowControlReturnEnd(fcr);
            
            while (stream_.nextTokenIs(E_LEMON)) {
                writer.writeCoin(stream_.consumeToken().value[0]);
                
                parseIfExpression(token);
                flowControlBlock();
                flowControlReturnEnd(fcr);
            }
            
            if (stream_.nextTokenIs(E_STRAWBERRY)) {
                writer.writeCoin(stream_.consumeToken().value[0]);
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
            
            parse(stream_.consumeToken(), token, typeBoolean);
            flowControlBlock();
            returned = false;
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            auto placeholder = writer.writeCoinPlaceholder();
            
            auto &variableToken = stream_.consumeToken(VARIABLE);
            
            if (scoper.currentScope().hasLocalVariable(variableToken.value)) {
                throw CompilerErrorException(variableToken, "Cannot redeclare variable.");
            }
            
            int vID = scoper.reserveVariableSlot();
            writer.writeCoin(vID);
            
            Type iteratee = parse(stream_.consumeToken(), token, typeSomeobject);
            
            Type itemType = typeNothingness;
            
            if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_LIST) {
                // If the iteratee is a list, the Real-Time Engine has some special sugar
                placeholder.write(0x65);
                writer.writeCoin(scoper.reserveVariableSlot());  //Internally needed
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(iteratee.genericArguments[0], vID, true, true, variableToken));
            }
            else if (iteratee.type() == TT_CLASS && iteratee.eclass == CL_RANGE) {
                // If the iteratee is a range, the Real-Time Engine also has some special sugar
                placeholder.write(0x66);
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(typeInteger, vID, true, true, variableToken));
            }
            else if (typeIsEnumerable(iteratee, &itemType)) {
                placeholder.write(0x64);
                writer.writeCoin(scoper.reserveVariableSlot());  //Internally needed
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
            writer.writeCoin(0x3C);
            if (initializer && !calledSuper && initializer->eclass->superclass) {
                throw CompilerErrorException(token, "Attempt to use 🐕 before superinitializer call.");
            }
            
            if (inClassContext) {
                throw CompilerErrorException(token, "Illegal use of 🐕.");
                break;
            }
            
            scoper.objectScope()->initializerUnintializedVariablesCheck(token,
                                                "Instance variable \"%s\" must be initialized before the use of 🐕.");
            
            return typeContext.normalType;
        }
        case E_UP_POINTING_RED_TRIANGLE: {
            writer.writeCoin(0x13);
            
            Type type = parseAndFetchType(typeContext, AllKindsOfDynamism);
            
            if (type.type() != TT_ENUM) {
                throw CompilerErrorException(token, "The given type cannot be accessed.");
            }
            else if (type.optional()) {
                throw CompilerErrorException(token, "Optionals cannot be accessed.");
            }
            
            auto name = stream_.consumeToken(IDENTIFIER);
            
            auto v = type.eenum->getValueFor(name.value[0]);
            if (!v.first) {
                ecCharToCharStack(name.value[0], valueName);
                ecCharToCharStack(type.eenum->name(), enumName);
                throw CompilerErrorException(name, "%s does not have a member named %s.", enumName, valueName);
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
        case E_HIGH_VOLTAGE_SIGN: {
            writer.writeCoin(0x17);
            return typeNothingness;
        }
        case E_CLOUD: {
            writer.writeCoin(0x2E);
            parse(stream_.consumeToken());
            return typeBoolean;
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writer.writeCoin(0x2D);
            
            parse(stream_.consumeToken(), token, typeSomeobject);
            parse(stream_.consumeToken(), token, typeSomeobject);
            
            return typeBoolean;
        }
        case E_GOAT: {
            if (!initializer) {
                throw CompilerErrorException(token, "🐐 can only be used inside initializers.");
                break;
            }
            if (!typeContext.normalType.eclass->superclass) {
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
            
            writer.writeCoin(0x3D);
            
            Class *eclass = typeContext.normalType.eclass;
            
            writer.writeCoin(eclass->superclass->index);
            
            auto &initializerToken = stream_.consumeToken(IDENTIFIER);
            
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
                throw CompilerErrorException(token, "🍎’s return may not be used as an argument.");
            }
            effect = true;
            
            writer.writeCoin(0x60);
            
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
            auto placeholder = writer.writeCoinPlaceholder();
            
            Type originalType = parse(stream_.consumeToken(), token, typeSomething);
            Type type = parseAndFetchType(typeContext, NoDynamism, nullptr);
            
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
                    writer.writeCoin(type.eclass->index);
                    break;
                }
                case TT_PROTOCOL:
                    placeholder.write(originalType.type() == TT_SOMETHING || originalType.optional() ? 0x45 : 0x41);
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
                    throw CompilerErrorException(token, "You cannot cast to %s.", typeString.c_str());
                }
            }
            
            type.setOptional();
            return type;
        }
        case E_BEER_MUG: {
            writer.writeCoin(0x3A);
            
            Type t = parse(stream_.consumeToken());
            
            if (!t.optional()) {
                throw CompilerErrorException(token, "🍺 can only be used with optionals.");
            }
            
            return t.copyWithoutOptional();
        }
        case E_CLINKING_BEER_MUGS: {
            writer.writeCoin(0x3B);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin();
            
            auto &methodToken = stream_.consumeToken();
            
            Type type = parse(stream_.consumeToken());
            if (!type.optional()) {
                throw CompilerErrorException(token, "🍻 may only be used on 🍬.");
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
            returnType.setOptional();
            return returnType.resolveOn(typeContext);
        }
        case E_HOT_PEPPER: {
            auto &methodName = stream_.consumeToken();
            
            writer.writeCoin(0x71);
            
            Type type = parse(stream_.consumeToken());
            
            if (type.type() != TT_CLASS) {
                throw CompilerErrorException(token, "You can only capture method calls on class instances.");
            }
            
            auto method = type.eclass->getMethod(methodName, type, typeContext);
            method->deprecatedWarning(methodName);
            
            writer.writeCoin(method->vti);
            
            return method->type();
        }
        case E_GRAPES: {
            writer.writeCoin(0x70);
            
            auto function = Closure(token.position());
            parseArgumentList(&function, typeContext);
            parseReturnType(&function, typeContext);
            parseBody(&function);
            
            auto variableCountPlaceholder = writer.writeCoinPlaceholder();
            auto coinCountPlaceholder = writer.writeCoinsCountPlaceholderCoin();
            
            auto flattenedResult = scoper.flattenedCopy(static_cast<int>(function.arguments.size()));
            auto closureScoper = flattenedResult.first;
            
            auto analyzer = StaticFunctionAnalyzer(function, package_, nullptr, inClassContext, typeContext,
                                                   writer, closureScoper);
            analyzer.analyze(true);
            
            coinCountPlaceholder.write();
            variableCountPlaceholder.write(closureScoper.numberOfReservations());
            writer.writeCoin(static_cast<EmojicodeCoin>(function.arguments.size())
                             | (analyzer.usedSelfInBody() ? 1 << 16 : 0));
            writer.writeCoin(flattenedResult.second);
            
            return function.type();
        }
        case E_LOLLIPOP: {
            writer.writeCoin(0x72);
            
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
            
            Class *superclass = typeContext.normalType.eclass->superclass;
            
            if (superclass == nullptr) {
                throw CompilerErrorException(token, "Class has no superclass.");
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
        case E_LARGE_BLUE_DIAMOND: {
            writer.writeCoin(0x4);
            
            TypeDynamism dynamism;
            Type type = parseAndFetchType(typeContext, AllKindsOfDynamism, &dynamism)
                        .resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (type.type() != TT_CLASS) {
                throw CompilerErrorException(token, "The given type cannot be initiatied.");
            }
            else if (type.optional()) {
                throw CompilerErrorException(token, "Optionals cannot be initiatied.");
            }
            
            if (dynamism) {
                writer.writeCoin(UINT32_MAX);
            }
            else {
                writer.writeCoin(type.eclass->index);
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
            
            writer.writeCoin(initializer->vti);
            
            checkAccess(initializer, token, "Initializer");
            checkArguments(initializer->arguments, type, token);
            
            if (initializer->canReturnNothingness) {
                type.setOptional();
            }
            return type;
        }
        case E_DOUGHNUT: {
            writer.writeCoin(0x2);
            
            auto &methodToken = stream_.consumeToken(IDENTIFIER);
            
            TypeDynamism dynamism;
            Type type = parseAndFetchType(typeContext, AllKindsOfDynamism, &dynamism)
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
        default: {
            auto placeholder = writer.writeCoinPlaceholder();
            
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
            
            if (!calledSuper && typeContext.normalType.eclass->superclass) {
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
    writer.writeUInt16(procedure->vti);
    writer.writeByte(static_cast<uint8_t>(procedure->arguments.size()));
    
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
    
    variableCountPlaceholder.write(scoper.numberOfReservations());
    coinsCountPlaceholder.write();
}