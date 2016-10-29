//
//  CallableParserAndGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CallableParserAndGenerator.hpp"
#include "Type.hpp"
#include "utf8.h"
#include "Class.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "Function.hpp"
#include "ValueType.hpp"
#include "CommonTypeFinder.hpp"
#include "VariableNotFoundErrorException.hpp"
#include "StringPool.hpp"

Type CallableParserAndGenerator::parse(const Token &token, const Token &parentToken,
                                   Type type, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = ctargs ? parse(token) : parse(token, type);
    if (!returnType.compatibleTo(type, typeContext, ctargs)) {
        auto cn = returnType.toString(typeContext, true);
        auto tn = type.toString(typeContext, true);
        throw CompilerErrorException(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

bool CallableParserAndGenerator::typeIsEnumerable(Type type, Type *elementType) {
    if (type.type() == TypeContent::Class && !type.optional()) {
        for (Class *a = type.eclass(); a != nullptr; a = a->superclass()) {
            for (auto protocol : a->protocols()) {
                if (protocol.protocol() == PR_ENUMERATEABLE) {
                    *elementType = protocol.resolveOn(type).genericArguments[0];
                    return true;
                }
            }
        }
    }
    return false;
}

Type CallableParserAndGenerator::parseFunctionCall(Type type, Function *p, const Token &token) {
    std::vector<Type> genericArguments;
    std::vector<CommonTypeFinder> genericArgsFinders;
    std::vector<Type> givenArgumentTypes;
    
    p->deprecatedWarning(token);
    
    while (stream_.nextTokenIs(E_SPIRAL_SHELL)) {
        stream_.consumeToken(TokenType::Identifier);
        
        auto type = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);
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
    if (stream_.nextTokenIs(TokenType::ArgumentBracketOpen)) {
        stream_.consumeToken(TokenType::ArgumentBracketOpen);
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
        stream_.consumeToken(TokenType::ArgumentBracketClose);
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
    
    if (p->accessLevel() == AccessLevel::Private) {
        if (this->typeContext.calleeType().type() != p->owningType().type()
                || p->owningType().typeDefinition() != this->typeContext.calleeType().typeDefinition()) {
            ecCharToCharStack(p->name(), nm);
            throw CompilerErrorException(token, "%s is 🔒.", nm);
        }
    }
    else if (p->accessLevel() == AccessLevel::Protected) {
        if (this->typeContext.calleeType().type() != p->owningType().type()
                || !this->typeContext.calleeType().eclass()->inheritsFrom(p->owningType().eclass())) {
            ecCharToCharStack(p->name(), nm);
            throw CompilerErrorException(token, "%s is 🔐.", nm);
        }
    }
    
    return p->returnType.resolveOn(typeContext);
}

void CallableParserAndGenerator::writeCoinForScopesUp(bool inObjectScope, EmojicodeCoin stack,
                                                  EmojicodeCoin object, SourcePosition p) {
    if (!inObjectScope) {
        writer.writeCoin(stack, p);
    }
    else {
        writer.writeCoin(object, p);
        usedSelf = true;
    }
}

void CallableParserAndGenerator::flowControlBlock(bool block) {
    scoper.currentScope().pushInitializationLevel();
    if (mode == CallableParserAndGeneratorMode::ObjectMethod ||
        mode == CallableParserAndGeneratorMode::ObjectInitializer) {
        scoper.objectScope()->pushInitializationLevel();
    }
    
    if (block) {
        scoper.pushScope();
    }
    
    flowControlDepth++;
    
    auto &token = stream_.consumeToken(TokenType::Identifier);
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
    
    if (block) {
        scoper.popScopeAndRecommendFrozenVariables();
    }
    
    scoper.currentScope().popInitializationLevel();
    if (mode == CallableParserAndGeneratorMode::ObjectMethod ||
        mode == CallableParserAndGeneratorMode::ObjectInitializer) {
        scoper.objectScope()->popInitializationLevel();
    }
    
    flowControlDepth--;
}

void CallableParserAndGenerator::flowControlReturnEnd(FlowControlReturn &fcr) {
    if (returned) {
        fcr.branchReturns++;
        returned = false;
    }
    fcr.branches++;
}

void CallableParserAndGenerator::parseIfExpression(const Token &token) {
    if (stream_.nextTokenIs(E_SOFT_ICE_CREAM)) {
        stream_.consumeToken(TokenType::Identifier);
        writer.writeCoin(0x3E, token);
        
        auto &varName = stream_.consumeToken(TokenType::Variable);
        if (scoper.currentScope().hasLocalVariable(varName.value)) {
            throw CompilerErrorException(token, "Cannot redeclare variable.");
        }
        
        int id = scoper.reserveVariableSlot();
        writer.writeCoin(id, token);
        
        Type t = parse(stream_.consumeToken());
        if (!t.optional()) {
            throw CompilerErrorException(token, "Condition assignment can only be used with optionals.");
        }
        
        t = t.copyWithoutOptional();
        
        scoper.currentScope().setLocalVariable(varName.value, Variable(t, id, 1, true, varName));
    }
    else {
        parse(stream_.consumeToken(TokenType::NoType), token, typeBoolean);
    }
}

Type CallableParserAndGenerator::parse(const Token &token, Type expectation) {
    switch (token.type()) {
        case TokenType::String: {
            writer.writeCoin(0x10, token);
            writer.writeCoin(StringPool::theStringPool().poolString(token.value), token);
            return Type(CL_STRING);
        }
        case TokenType::BooleanTrue:
            writer.writeCoin(0x11, token);
            return typeBoolean;
        case TokenType::BooleanFalse:
            writer.writeCoin(0x12, token);
            return typeBoolean;
        case TokenType::Integer: {
            /* We know token->value only contains ints less than 255 */
            const char *string = token.value.utf8CString();
            
            EmojicodeInteger l = strtoll(string, nullptr, 0);
            delete [] string;
            
            if (expectation.type() == TypeContent::ValueType && expectation.valueType() == VT_DOUBLE) {
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
        case TokenType::Double: {
            writer.writeCoin(0x15, token);
            
            const char *string = token.value.utf8CString();
            
            double d = strtod(string, nullptr);
            delete [] string;
            writer.writeDoubleCoin(d, token);
            return typeFloat;
        }
        case TokenType::Symbol:
            writer.writeCoin(0x16, token);
            writer.writeCoin(token.value[0], token);
            return typeSymbol;
        case TokenType::Variable: {
            auto var = scoper.getVariable(token.value, token.position());
            
            var.first.uninitalizedError(token);
            
            writeCoinForScopesUp(var.second, 0x1A, 0x1C, token);
            writer.writeCoin(var.first.id(), token);
            
            return var.first.type;
        }
        case TokenType::Identifier:
            return parseIdentifier(token, expectation);
        case TokenType::DocumentationComment:
            throw CompilerErrorException(token, "Misplaced documentation comment.");
        case TokenType::ArgumentBracketOpen:
            throw CompilerErrorException(token, "Unexpected 〖");
        case TokenType::ArgumentBracketClose:
            throw CompilerErrorException(token, "Unexpected 〗");
        case TokenType::NoType:
        case TokenType::Comment:
            break;
    }
    throw CompilerErrorException(token, "Cannot determine expression’s return type.");
}

Type CallableParserAndGenerator::parseIdentifier(const Token &token, Type expectation) {
    if (token.value[0] != E_RED_APPLE) {
        // We need a chance to test whether the red apple’s return is used
        effect = true;
    }

    switch (token.value[0]) {
        case E_SHORTCAKE: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            if (scoper.currentScope().hasLocalVariable(varName.value)) {
                throw CompilerErrorException(token, "Cannot redeclare variable.");
            }
            
            Type t = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);
            
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
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            Type type = typeNothingness;
            try {
                auto var = scoper.getVariable(varName.value, varName.position());
                if (var.first.initialized <= 0) {
                    var.first.initialized = 1;
                }
                
                var.first.mutate(varName);
                
                writeCoinForScopesUp(var.second, 0x1B, 0x1D, token);
                writer.writeCoin(var.first.id(), token);
                
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
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
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
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
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
            
            writer.writeCoin(var.first.id(), token);
            
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
            stream_.consumeToken(TokenType::Identifier);
            
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
            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_LIST) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, listType);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    ct.addType(parse(stream_.consumeToken()), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = ct.getCommonType(token);
            }
            
            placeholder.write();
            return type;
        }
        case E_HONEY_POT: {
            writer.writeCoin(0x50, token);
           
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            Type type = Type(CL_DICTIONARY);
            
            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_DICTIONARY) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, Type(CL_STRING));
                    parse(stream_.consumeToken(), token, listType);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, Type(CL_STRING));
                    ct.addType(parse(stream_.consumeToken()), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
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
            
            scoper.pushScope();
            parseIfExpression(token);
            flowControlBlock(false);
            scoper.popScopeAndRecommendFrozenVariables();
            flowControlReturnEnd(fcr);
            
            while (stream_.nextTokenIs(E_LEMON)) {
                stream_.consumeToken();
                writer.writeCoin(0x63, token);
                
                scoper.pushScope();
                parseIfExpression(token);
                flowControlBlock(false);
                scoper.popScopeAndRecommendFrozenVariables();
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
            
            parseIfExpression(token);
            flowControlBlock();
            returned = false;
            
            return typeNothingness;
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            auto &variableToken = stream_.consumeToken(TokenType::Variable);
            
            scoper.pushScope();
            
            int vID = scoper.reserveVariableSlot();
            writer.writeCoin(vID, token);
            
            Type iteratee = parse(stream_.consumeToken(), token, typeSomeobject);
            
            Type itemType = typeNothingness;
            
            if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_LIST) {
                // If the iteratee is a list, the Real-Time Engine has some special sugar
                placeholder.write(0x65);
                writer.writeCoin(scoper.reserveVariableSlot(), token);  //Internally needed
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(iteratee.genericArguments[0], vID, true, true, variableToken));
            }
            else if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_RANGE) {
                // If the iteratee is a range, the Real-Time Engine also has some special sugar
                placeholder.write(0x66);
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(typeInteger, vID, true, true, variableToken));
            }
            else if (typeIsEnumerable(iteratee, &itemType)) {
                Function *iteratorMethod = iteratee.eclass()->lookupMethod(E_DANGO);
                iteratorMethod->vtiForUse();
                TypeDefinitionFunctional *td = iteratorMethod->returnType.typeDefinitionFunctional();
                td->lookupMethod(E_DOWN_POINTING_SMALL_RED_TRIANGLE)->vtiForUse();
                td->lookupMethod(E_RED_QUESTION_MARK)->vtiForUse();
                placeholder.write(0x64);
                writer.writeCoin(scoper.reserveVariableSlot(), token);  //Internally needed
                scoper.currentScope().setLocalVariable(variableToken.value, Variable(itemType, vID, true, true, variableToken));
            }
            else {
                auto iterateeString = iteratee.toString(typeContext, true);
                throw CompilerErrorException(token, "%s does not conform to s🔂.", iterateeString.c_str());
            }
            
            flowControlBlock(false);
            returned = false;
            scoper.popScopeAndRecommendFrozenVariables();
            
            return typeNothingness;
        }
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE: {
            ecCharToCharStack(token.value[0], identifier);
            throw CompilerErrorException(token, "Unexpected identifier %s.", identifier);
            return typeNothingness;
        }
        case E_DOG: {
            usedSelf = true;
            writer.writeCoin(0x3C, token);
            if (mode == CallableParserAndGeneratorMode::ObjectInitializer) {
                if (!calledSuper && static_cast<Initializer &>(callable).owningType().eclass()->superclass()) {
                    throw CompilerErrorException(token, "Attempt to use 🐕 before superinitializer call.");
                }
                
                scoper.objectScope()->initializerUnintializedVariablesCheck(token,
                                                                            "Instance variable \"%s\" must be initialized before the use of 🐕.");
            }
            
            if (mode == CallableParserAndGeneratorMode::Function ||
                mode == CallableParserAndGeneratorMode::ClassMethod) {
                throw CompilerErrorException(token, "Illegal use of 🐕.");
            }
            
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
            if (mode != CallableParserAndGeneratorMode::ObjectInitializer) {
                throw CompilerErrorException(token, "🐐 can only be used inside initializers.");
            }
            if (!typeContext.calleeType().eclass()->superclass()) {
                throw CompilerErrorException(token, "🐐 can only be used if the eclass inherits from another.");
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
            
            Class *eclass = typeContext.calleeType().eclass();
            
            writer.writeCoin(0xF, token);
            writer.writeCoin(eclass->superclass()->index, token);
            
            auto &initializerToken = stream_.consumeToken(TokenType::Identifier);
            
            auto initializer = eclass->superclass()->getInitializer(initializerToken, Type(eclass), typeContext);
            
            writer.writeCoin(initializer->vtiForUse(), token);
            
            parseFunctionCall(typeContext.calleeType(), initializer, token);

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
            
            if (mode == CallableParserAndGeneratorMode::ObjectInitializer) {
                if (static_cast<Initializer &>(callable).canReturnNothingness) {
                    parse(stream_.consumeToken(), token, typeNothingness);
                    return typeNothingness;
                }
                else {
                    throw CompilerErrorException(token, "🍎 cannot be used inside an initializer.");
                }
            }
            
            parse(stream_.consumeToken(), token, callable.returnType);
            returned = true;
            return typeNothingness;
        }
        case E_WHITE_LARGE_SQUARE: {
            writer.writeCoin(0xE, token);
            Type originalType = parse(stream_.consumeToken());
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerErrorException(token, "Can’t get metatype of %s.", string.c_str());
            }
            originalType.setMeta(true);
            return originalType;
        }
        case E_WHITE_SQUARE_BUTTON: {
            writer.writeCoin(0xF, token);
            Type originalType = parseTypeDeclarative(typeContext, TypeDynamism::None);
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerErrorException(token, "Can’t get metatype of %s.", string.c_str());
            }
            writer.writeCoin(originalType.eclass()->index, token);
            originalType.setMeta(true);
            return originalType;
        }
        case E_BLACK_SQUARE_BUTTON: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            Type originalType = parse(stream_.consumeToken());
            auto pair = parseTypeAsValue(typeContext, token);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (originalType.compatibleTo(type, typeContext)) {
                compilerWarning(token, "Superfluous cast.");
            }
            else if (!type.compatibleTo(originalType, typeContext)) {
                auto typeString = type.toString(typeContext, true);
                compilerWarning(token, "Cast to unrelated type %s will always fail.", typeString.c_str());
            }
            
            if (type.type() == TypeContent::Class) {
                auto offset = type.eclass()->numberOfGenericArgumentsWithSuperArguments() - type.eclass()->numberOfOwnGenericArguments();
                for (size_t i = 0; i < type.eclass()->numberOfOwnGenericArguments(); i++) {
                    if(!type.eclass()->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments[i], type) ||
                       !type.genericArguments[i].compatibleTo(type.eclass()->genericArgumentConstraints()[offset + i], type)) {
                        throw CompilerErrorException(token, "Dynamic casts involving generic type arguments are not possible yet. Please specify the generic argument constraints of the class for compatibility with future versions.");
                    }
                }
                
                placeholder.write(originalType.type() == TypeContent::Something || originalType.optional() ? 0x44 : 0x40);
            }
            else if (type.type() == TypeContent::Protocol && isStatic(pair.second)) {
                placeholder.write(originalType.type() == TypeContent::Something || originalType.optional() ? 0x45 : 0x41);
                writer.writeCoin(type.protocol()->index, token);
            }
            else if (type.type() == TypeContent::ValueType && isStatic(pair.second)) {
                if (type.valueType() == VT_BOOLEAN) {
                    placeholder.write(0x42);
                }
                else if (type.valueType() == VT_INTEGER) {
                    placeholder.write(0x43);
                }
                else if (type.valueType() == VT_SYMBOL) {
                    placeholder.write(0x46);
                }
                else if (type.valueType() == VT_DOUBLE) {
                    placeholder.write(0x47);
                }
            }
            else {
                auto typeString = pair.first.toString(typeContext, true);
                throw CompilerErrorException(token, "You cannot cast to %s.", typeString.c_str());
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
            
            auto &methodToken = stream_.consumeToken();
            
            Type type = parse(stream_.consumeToken());
            if (!type.optional()) {
                throw CompilerErrorException(token, "🍻 may only be used on 🍬.");
            }
            
            auto method = type.eclass()->getMethod(methodToken, type, typeContext);
            
            writer.writeCoin(method->vtiForUse(), token);
            
            auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
            parseFunctionCall(type, method, token);
            
            placeholder.write();
            
            Type returnType = method->returnType;
            returnType.setOptional();
            return returnType.resolveOn(typeContext);
        }
        case E_HOT_PEPPER: {
            Function *function;
            
            auto placeholder = writer.writeCoinPlaceholder(token);
            if (stream_.nextTokenIs(E_DOUGHNUT)) {
                stream_.consumeToken();
                auto &methodName = stream_.consumeToken();
                auto pair = parseTypeAsValue(typeContext, token);
                auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);
                
                if (type.type() == TypeContent::Class) {
                    placeholder.write(0x73);
                }
                else if (type.type() == TypeContent::ValueType) {
                    notStaticError(pair.second, token, "Value Types");
                    placeholder.write(0x74);
                    writer.writeCoin(0x17, token);
                }
                else {
                    throw CompilerErrorException(token, "You can’t capture method calls on this kind of type.");
                }
                
                function = type.eclass()->getClassMethod(methodName, type, typeContext);
            }
            else {
                auto &methodName = stream_.consumeToken();
                Type type = parse(stream_.consumeToken());
                
                if (type.type() == TypeContent::Class) {
                    placeholder.write(0x72);
                }
                else if (type.type() == TypeContent::ValueType) {
                    placeholder.write(0x74);
                }
                else {
                    throw CompilerErrorException(token, "You can’t capture method calls on this kind of type.");
                }
                function = type.typeDefinitionFunctional()->getMethod(methodName, type, typeContext);
            }

            function->deprecatedWarning(token);
            writer.writeCoin(function->vtiForUse(), token);
            return function->type();
        }
        case E_GRAPES: {
            writer.writeCoin(0x71, token);
            
            auto function = Closure(token.position());
            parseArgumentList(&function, typeContext);
            parseReturnType(&function, typeContext);
            parseBody(&function);
            
            auto variableCountPlaceholder = writer.writeCoinPlaceholder(token);
            auto coinCountPlaceholder = writer.writeCoinsCountPlaceholderCoin(token);
            
            auto flattenedResult = scoper.flattenedCopy(static_cast<int>(function.arguments.size()));
            auto closureScoper = flattenedResult.first;
            
            auto analyzer = CallableParserAndGenerator(function, package_, mode, typeContext, writer, closureScoper);  // TODO: Intializer
            analyzer.analyze(true);
            
            coinCountPlaceholder.write();
            variableCountPlaceholder.write(closureScoper.maxVariableCount());
            writer.writeCoin(static_cast<EmojicodeCoin>(function.arguments.size())
                             | (analyzer.usedSelfInBody() ? 1 << 16 : 0), token);
            writer.writeCoin(flattenedResult.second, token);
            
            return function.type();
        }
        case E_LOLLIPOP: {
            writer.writeCoin(0x70, token);
            
            Type type = parse(stream_.consumeToken());
            
            if (type.type() != TypeContent::Callable) {
                throw CompilerErrorException(token, "Given value is not callable.");
            }
            
            for (int i = 1; i < type.genericArguments.size(); i++) {
                parse(stream_.consumeToken(), token, type.genericArguments[i]);
            }
            
            return type.genericArguments[0];
        }
        case E_CHIPMUNK: {
            auto &nameToken = stream_.consumeToken();
            
            if (mode != CallableParserAndGeneratorMode::ObjectMethod) {
                throw CompilerErrorException(token, "Not within an object-context.");
            }
            
            Class *superclass = typeContext.calleeType().eclass()->superclass();
            
            if (superclass == nullptr) {
                throw CompilerErrorException(token, "Class has no superclass.");
            }
            
            Function *method = superclass->getMethod(nameToken, Type(superclass), typeContext);
            
            writer.writeCoin(0x5, token);
            writer.writeCoin(0xF, token);
            writer.writeCoin(superclass->index, token);
            writer.writeCoin(method->vtiForUse(), token);
            
            return parseFunctionCall(typeContext.calleeType(), method, token);
        }
        case E_LARGE_BLUE_DIAMOND: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            auto pair = parseTypeAsValue(typeContext, token, expectation);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);
            
            auto &initializerName = stream_.consumeToken(TokenType::Identifier);
            
            if (type.optional()) {
                throw CompilerErrorException(token, "Optionals cannot be instantiated.");
            }
            
            if (type.type() == TypeContent::Enum) {
                notStaticError(pair.second, token, "Enums");
                placeholder.write(0x13);
                
                auto v = type.eenum()->getValueFor(initializerName.value[0]);
                if (!v.first) {
                    ecCharToCharStack(initializerName.value[0], valueName);
                    ecCharToCharStack(type.eenum()->name(), enumName);
                    throw CompilerErrorException(initializerName, "%s does not have a member named %s.",
                                                 enumName, valueName);
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
            else if (type.type() == TypeContent::ValueType) {
                notStaticError(pair.second, token, "Value Types");
                placeholder.write(0x7);
                
                auto initializer = type.valueType()->getInitializer(initializerName, type, typeContext);
                writer.writeCoin(initializer->vtiForUse(), token);
                parseFunctionCall(type, initializer, token);
                if (initializer->canReturnNothingness) {
                    type.setOptional();
                }
                return type;
            }
            else if (type.type() == TypeContent::Class) {
                placeholder.write(0x4);
                
                Initializer *initializer = type.eclass()->getInitializer(initializerName, type, typeContext);
                
                if (!isStatic(pair.second) && !initializer->required) {
                    throw CompilerErrorException(initializerName,
                                                 "Only required initializers can be used with dynamic types.");
                }
                
                initializer->deprecatedWarning(initializerName);
                
                writer.writeCoin(initializer->vtiForUse(), token);
                
                parseFunctionCall(type, initializer, token);
                
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
            auto &methodToken = stream_.consumeToken(TokenType::Identifier);
            
            auto placeholder = writer.writeCoinPlaceholder(token);
            auto pair = parseTypeAsValue(typeContext, token);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);
            
            if (type.optional()) {
                compilerWarning(token, "You cannot call optionals on 🍬.");
            }

            Function *method;
            if (type.type() == TypeContent::Class) {
                placeholder.write(0x2);
                method = type.typeDefinitionFunctional()->getClassMethod(methodToken, type, typeContext);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::ValueType && isStatic(pair.second)) {
                method = type.typeDefinitionFunctional()->getClassMethod(methodToken, type, typeContext);
                placeholder.write(0x7);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else {
                throw CompilerErrorException(token, "You can’t call type methods on %s.",
                                             pair.first.toString(typeContext, true).c_str());
            }
            return parseFunctionCall(type, method, token);
        }
        default: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            auto &tobject = stream_.consumeToken();
            Type type = parse(tobject).resolveOnSuperArgumentsAndConstraints(typeContext);
            if (type.optional()) {
                throw CompilerErrorException(tobject, "You cannot call methods on optionals.");
            }
            
            Function *method;
            if (type.type() == TypeContent::ValueType) {
                if (type.valueType() == VT_BOOLEAN) {
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
                else if (type.valueType() == VT_INTEGER) {
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
                        case E_LEFT_POINTING_BACKHAND_INDEX:
                            placeholder.write(0x5E);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                        case E_RIGHT_POINTING_BACKHAND_INDEX:
                            placeholder.write(0x5F);
                            parse(stream_.consumeToken(), token, typeInteger);
                            return typeInteger;
                    }
                }
                else if (type.valueType() == VT_DOUBLE) {
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
                
                method = type.valueType()->getMethod(token, type, typeContext);
                placeholder.write(0x6);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Protocol) {
                method = type.protocol()->getMethod(token, type, typeContext);
                placeholder.write(0x3);
                writer.writeCoin(type.protocol()->index, token);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Enum && token.value[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
                parse(stream_.consumeToken(), token, type);  // Must be of the same type as the callee
                placeholder.write(0x20);
                return typeBoolean;
            }
            else if (type.type() == TypeContent::Class) {
                method = type.eclass()->getMethod(token, type, typeContext);
                placeholder.write(0x1);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else {
                auto typeString = type.toString(typeContext, true);
                throw CompilerErrorException(token, "You cannot call methods on %s.", typeString.c_str());
            }
            
            return parseFunctionCall(type, method, token);
        }
    }
    return typeNothingness;
}

void CallableParserAndGenerator::noReturnError(SourcePosition p) {
    if (callable.returnType.type() != TypeContent::Nothingness && !returned) {
        throw CompilerErrorException(p, "An explicit return is missing.");
    }
}

void CallableParserAndGenerator::noEffectWarning(const Token &warningToken) {
    if (!effect) {
        compilerWarning(warningToken, "Statement seems to have no effect whatsoever.");
    }
}

void CallableParserAndGenerator::notStaticError(TypeAvailability t, SourcePosition p, const char *name) {
    if (!isStatic(t)) {
        throw CompilerErrorException(p, "%s cannot be used dynamically.", name);
    }
}

std::pair<Type, TypeAvailability> CallableParserAndGenerator::parseTypeAsValue(TypeContext tc, SourcePosition p,
                                                                           Type expectation) {
    if (stream_.nextTokenIs(E_BLACK_LARGE_SQUARE)) {
        stream_.consumeToken();
        auto type = parse(stream_.consumeToken());
        if (!type.meta()) {
            throw CompilerErrorException(p, "Expected meta type.");
        }
        if (type.optional()) {
            throw CompilerErrorException(p, "🍬 can’t be used as meta type.");
        }
        type.setMeta(false);
        return std::pair<Type, TypeAvailability>(type, TypeAvailability::DynamicAndAvailabale);
    }
    Type ot = parseTypeDeclarative(tc, TypeDynamism::AllKinds, expectation);
    switch (ot.type()) {
        case TypeContent::Reference:
            throw CompilerErrorException(p, "Generic Arguments are not yet available for reflection.");
        case TypeContent::Class:
            writer.writeCoin(0xF, p);
            writer.writeCoin(ot.eclass()->index, p);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndAvailabale);
        case TypeContent::Self:
            if (mode != CallableParserAndGeneratorMode::ClassMethod) {
                throw CompilerErrorException(p, "Illegal use of 🐕.");
            }
            writer.writeCoin(0x3C, p);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::DynamicAndAvailabale);
        case TypeContent::LocalReference:
            throw CompilerErrorException(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndUnavailable);
}

CallableParserAndGenerator::CallableParserAndGenerator(Callable &callable, Package *p,
                                                       CallableParserAndGeneratorMode mode,
                                                       TypeContext typeContext, CallableWriter &writer,
                                                       CallableScoper &scoper)
        : AbstractParser(p, callable.tokenStream()),
          mode(mode),
          callable(callable),
          writer(writer),
          scoper(scoper),
          typeContext(typeContext) {}

void CallableParserAndGenerator::analyze(bool compileDeadCode) {
    auto isClassInitializer = mode == CallableParserAndGeneratorMode::ObjectInitializer;
    if (mode == CallableParserAndGeneratorMode::ObjectMethod ||
        mode == CallableParserAndGeneratorMode::ObjectInitializer) {
        scoper.objectScope()->setVariableInitialization(!isClassInitializer);
    }
    try {
        Scope &methodScope = scoper.pushScope();
        for (size_t i = 0; i < callable.arguments.size(); i++) {
            auto variable = callable.arguments[i];
            methodScope.setLocalVariable(variable.name.value, Variable(variable.type, static_cast<int>(i), true,
                                                                       true, variable.name));
        }
        scoper.ensureNReservations(static_cast<int>(callable.arguments.size()));
        
        if (isClassInitializer) {
            auto initializer = static_cast<Initializer &>(callable);
            for (auto &var : initializer.argumentsToVariables()) {
                if (scoper.objectScope()->hasLocalVariable(var) == 0) {
                    throw CompilerErrorException(initializer.position(),
                                                 "🍼 was applied to \"%s\" but no matching instance variable was found.",
                                                 var.utf8CString());
                }
                auto &instanceVariable = scoper.objectScope()->getLocalVariable(var);
                auto &argumentVariable = methodScope.getLocalVariable(var);
                if (!argumentVariable.type.compatibleTo(instanceVariable.type, typeContext)) {
                    throw CompilerErrorException(initializer.position(),
                                                 "🍼 was applied to \"%s\" but instance variable has incompatible type.",
                                                 var.utf8CString());
                }
                instanceVariable.initialized = 1;
                writer.writeCoin(0x1D, initializer.position());
                writer.writeCoin(instanceVariable.id(), initializer.position());
                writer.writeCoin(0x1A, initializer.position());
                writer.writeCoin(argumentVariable.id(), initializer.position());
            }
        }
        
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
        
        if (isClassInitializer) {
            auto initializer = static_cast<Initializer &>(callable);
            scoper.objectScope()->initializerUnintializedVariablesCheck(initializer.position(),
                                                                    "Instance variable \"%s\" must be initialized.");
            
            if (!calledSuper && typeContext.calleeType().eclass()->superclass()) {
                ecCharToCharStack(initializer.name(), initializerName);
                throw CompilerErrorException(initializer.position(),
                              "Missing call to superinitializer in initializer %s.", initializerName);
            }
        }
        else {
            noReturnError(callable.position());
        }
    }
    catch (CompilerErrorException &ce) {
        printError(ce);
    }
}

void CallableParserAndGenerator::writeAndAnalyzeFunction(Function *function, CallableWriter &writer, Type classType,
                                                     CallableScoper &scoper, CallableParserAndGeneratorMode mode) {
    auto sca = CallableParserAndGenerator(*function, function->package(), mode, TypeContext(classType, function),
                                          writer, scoper);
    sca.analyze();
    function->setMaxVariableCount(scoper.maxVariableCount());
}
