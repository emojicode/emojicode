//
//  CallableParserAndGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstdlib>
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
#include "EmojicodeInstructions.h"
#include "CapturingCallableScoper.hpp"

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
    
    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
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
        auto resolved = var.type.resolveOn(typeContext);
        if (inferGenericArguments) {
            writer.writeCoin(resolved.size(), token);
            givenArgumentTypes.push_back(parse(stream_.consumeToken(), token, resolved, &genericArgsFinders));
        }
        else {
            writer.writeCoin(resolved.size(), token);
            parse(stream_.consumeToken(), token, resolved);
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
            throw CompilerErrorException(token, "%s is 🔒.", p->name().utf8().c_str());
        }
    }
    else if (p->accessLevel() == AccessLevel::Protected) {
        if (this->typeContext.calleeType().type() != p->owningType().type()
                || !this->typeContext.calleeType().eclass()->inheritsFrom(p->owningType().eclass())) {
            throw CompilerErrorException(token, "%s is 🔐.", p->name().utf8().c_str());
        }
    }
    
    return p->returnType.resolveOn(typeContext);
}

void CallableParserAndGenerator::writeCoinForStackOrInstance(bool inObjectScope, EmojicodeCoin stack,
                                                             EmojicodeCoin object, SourcePosition p) {
    if (!inObjectScope) {
        writer.writeCoin(stack, p);
    }
    else {
        writer.writeCoin(object, p);
        usedSelf = true;
    }
}

void CallableParserAndGenerator::parseCoinInBlock() {
    effect = false;

    auto insertionPoint = writer.getInsertionPoint();

    auto &token = stream_.consumeToken();
    auto type = parse(token);
    noEffectWarning(token);

    if (type.type() != TypeContent::Nothingness) {
        insertionPoint.insert(INS_DISCARD_PRODUCTION);
    }
}

void CallableParserAndGenerator::copyVariableContent(const Variable &variable, bool inObjectScope, SourcePosition p) {
    if (variable.type.size() == 1) {
        writeCoinForStackOrInstance(inObjectScope, INS_COPY_SINGLE_STACK, INS_COPY_SINGLE_INSTANCE, p);
        writer.writeCoin(variable.id(), p);
    }
    else {
        writeCoinForStackOrInstance(inObjectScope, INS_COPY_WITH_SIZE_STACK, INS_COPY_WITH_SIZE_INSTANCE, p);
        writer.writeCoin(variable.id(), p);
        writer.writeCoin(variable.type.size(), p);
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

    auto &token = stream_.requireIdentifier(E_GRAPES);

    auto placeholder = writer.writeCoinsCountPlaceholderCoin(token);
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        parseCoinInBlock();
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
    if (stream_.consumeTokenIf(E_SOFT_ICE_CREAM)) {
        writer.writeCoin(0x3E, token);
        
        auto &varName = stream_.consumeToken(TokenType::Variable);
        if (scoper.currentScope().hasLocalVariable(varName.value())) {
            throw CompilerErrorException(token, "Cannot redeclare variable.");
        }
        
        auto placeholder = writer.writeCoinPlaceholder(token);
        
        Type t = parse(stream_.consumeToken());
        if (!t.optional()) {
            throw CompilerErrorException(token, "Condition assignment can only be used with optionals.");
        }
        
        t = t.copyWithoutOptional();
        
        int id = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position(), true).id();
        placeholder.write(id);
    }
    else {
        parse(stream_.consumeToken(TokenType::NoType), token, Type::boolean());
    }
}

Type CallableParserAndGenerator::parse(const Token &token, Type expectation) {
    switch (token.type()) {
        case TokenType::String: {
            writer.writeCoin(INS_GET_STRING_POOL, token);
            writer.writeCoin(StringPool::theStringPool().poolString(token.value()), token);
            return Type(CL_STRING);
        }
        case TokenType::BooleanTrue:
            writer.writeCoin(INS_GET_TRUE, token);
            return Type::boolean();
        case TokenType::BooleanFalse:
            writer.writeCoin(INS_GET_FALSE, token);
            return Type::boolean();
        case TokenType::Integer: {
            long long value = std::stoll(token.value().utf8(), 0, 0);

            if (expectation.type() == TypeContent::ValueType && expectation.valueType() == VT_DOUBLE) {
                writer.writeCoin(INS_GET_DOUBLE, token);
                writer.writeDoubleCoin(value, token);
                return Type::doubl();
            }
            
            if (std::llabs(value) > INT32_MAX) {
                writer.writeCoin(INS_GET_64_INTEGER, token);
                
                writer.writeCoin(value >> 32, token);
                writer.writeCoin(static_cast<EmojicodeCoin>(value), token);
                
                return Type::integer();
            }
            else {
                writer.writeCoin(INS_GET_32_INTEGER, token);
                writer.writeCoin(static_cast<EmojicodeCoin>(value), token);
                
                return Type::integer();
            }
        }
        case TokenType::Double: {
            writer.writeCoin(INS_GET_DOUBLE, token);
            
            double d = std::stod(token.value().utf8());
            writer.writeDoubleCoin(d, token);

            return Type::doubl();
        }
        case TokenType::Symbol:
            writer.writeCoin(INS_GET_SYMBOL, token);
            writer.writeCoin(token.value()[0], token);
            return Type::symbol();
        case TokenType::Variable: {
            auto var = scoper.getVariable(token.value(), token.position());
            
            var.first.uninitalizedError(token);
            copyVariableContent(var.first, var.second, token.position());
            
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
    throw std::logic_error("Cannot determine expression’s return type.");
}

Type CallableParserAndGenerator::parseIdentifier(const Token &token, Type expectation) {
    if (!token.isIdentifier(E_RED_APPLE)) {
        // We need a chance to test whether the red apple’s return is used
        effect = true;
    }

    switch (token.value()[0]) {
        case E_SHORTCAKE: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            if (scoper.currentScope().hasLocalVariable(varName.value())) {
                throw CompilerErrorException(token, "Cannot redeclare variable.");
            }
            
            Type t = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);
            
            
            auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, false, varName.position(),
                                                               t.optional());
            
            if (t.optional()) {
                writer.writeCoin(INS_PRODUCE_WITH_STACK_DESTINATION, token);
                writer.writeCoin(var.id(), token);
                writer.writeCoin(INS_GET_NOTHINGNESS, token);
            }
            return Type::nothingness();
        }
        case E_CUSTARD: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            Type type = Type::nothingness();
            try {
                auto var = scoper.getVariable(varName.value(), varName.position());
                if (var.first.initialized <= 0) {
                    var.first.initialized = 1;
                }
                
                var.first.mutate(varName);
                
                writeCoinForStackOrInstance(var.second, INS_PRODUCE_WITH_STACK_DESTINATION,
                                            INS_PRODUCE_WITH_INSTANCE_DESTINATION, token);
                writer.writeCoin(var.first.id(), token);
                
                type = var.first.type;
            }
            catch (VariableNotFoundErrorException &vne) {
                // Not declared, declaring as local variable
                writer.writeCoin(INS_PRODUCE_WITH_STACK_DESTINATION, token);
                
                auto placeholder = writer.writeCoinPlaceholder(token);
                
                Type t = parse(stream_.consumeToken());
                int id = scoper.currentScope().setLocalVariable(varName.value(), t, false, varName.position(), true).id();
                placeholder.write(id);
                return Type::nothingness();
            }

            parse(stream_.consumeToken(), token, type);
            
            return Type::nothingness();
        }
        case E_SOFT_ICE_CREAM: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            if (scoper.currentScope().hasLocalVariable(varName.value())) {
                throw CompilerErrorException(token, "Cannot redeclare variable.");
            }
            
            writer.writeCoin(INS_PRODUCE_WITH_STACK_DESTINATION, token);
            
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            Type t = parse(stream_.consumeToken());
            int id = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position(), true).id();
            placeholder.write(id);
            return Type::nothingness();
        }
        case E_COOKING:
        case E_CHOCOLATE_BAR: {
            auto &varName = stream_.consumeToken(TokenType::Variable);
            
            auto var = scoper.getVariable(varName.value(), varName.position());
            
            var.first.uninitalizedError(varName);
            var.first.mutate(varName);
            
            if (!var.first.type.compatibleTo(Type::integer(), typeContext)) {
                throw CompilerErrorException(token, "%s can only operate on 🚂 variables.",
                                             token.value().utf8().c_str());
            }
            
            writeCoinForStackOrInstance(var.second, INS_PRODUCE_WITH_STACK_DESTINATION,
                                        INS_PRODUCE_WITH_INSTANCE_DESTINATION, token);
            writer.writeCoin(var.first.id(), token);
            
            if (token.isIdentifier(E_COOKING)) {
                writer.writeCoin(INS_DECREMENT, token);
            }
            else {
                writer.writeCoin(INS_INCREMENT, token);
            }
            return Type::nothingness();
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
            parse(stream_.consumeToken(), token, Type::integer());
            parse(stream_.consumeToken(), token, Type::integer());
            return Type(CL_RANGE);
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            writer.writeCoin(0x54, token);
            parse(stream_.consumeToken(), token, Type::integer());
            parse(stream_.consumeToken(), token, Type::integer());
            parse(stream_.consumeToken(), token, Type::integer());
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
            
            while (stream_.consumeTokenIf(E_LEMON)) {
                writer.writeCoin(0x63, token);
                
                scoper.pushScope();
                parseIfExpression(token);
                flowControlBlock(false);
                scoper.popScopeAndRecommendFrozenVariables();
                flowControlReturnEnd(fcr);
            }
            
            if (stream_.consumeTokenIf(E_STRAWBERRY)) {
                flowControlBlock();
                flowControlReturnEnd(fcr);
            }
            else {
                fcr.branches++;  // The else branch always exists. Theoretically at least.
            }
            
            placeholder.write();
            
            returned = fcr.returned();
            
            return Type::nothingness();
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
            writer.writeCoin(INS_REPEAT_WHILE, token);
            
            parseIfExpression(token);
            flowControlBlock();
            returned = false;
            
            return Type::nothingness();
        }
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
            auto placeholder = writer.writeCoinPlaceholder(token);
            
            auto &variableToken = stream_.consumeToken(TokenType::Variable);
            
            scoper.pushScope();
            
            auto valueVariablePlaceholder = writer.writeCoinPlaceholder(token);
            
            Type iteratee = parse(stream_.consumeToken(), token, Type::someobject());
            
            Type itemType = Type::nothingness();
            
            if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_LIST) {
                // If the iteratee is a list, the Real-Time Engine has some special sugar
                placeholder.write(0x65);
                int internalId = scoper.currentScope().setLocalVariable(EmojicodeString(), iteratee, false,
                                                                 variableToken.position()).id();
                writer.writeCoin(internalId, token);  //Internally needed
                int id = scoper.currentScope().setLocalVariable(variableToken.value(), iteratee.genericArguments[0],
                                                                true, variableToken.position(), true).id();
                valueVariablePlaceholder.write(id);
            }
            else if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_RANGE) {
                // If the iteratee is a range, the Real-Time Engine also has some special sugar
                placeholder.write(0x66);
                int id = scoper.currentScope().setLocalVariable(variableToken.value(), Type::integer(), true,
                                                                variableToken.position(), true).id();
                valueVariablePlaceholder.write(id);
            }
            else if (typeIsEnumerable(iteratee, &itemType)) {
                Function *iteratorMethod = iteratee.eclass()->lookupMethod(EmojicodeString(E_DANGO));
                iteratorMethod->vtiForUse();
                TypeDefinitionFunctional *td = iteratorMethod->returnType.typeDefinitionFunctional();
                td->lookupMethod(EmojicodeString(E_DOWN_POINTING_SMALL_RED_TRIANGLE))->vtiForUse();
                td->lookupMethod(EmojicodeString(E_RED_QUESTION_MARK))->vtiForUse();
                placeholder.write(INS_FOREACH);
                int internalId = scoper.currentScope().setLocalVariable(EmojicodeString(), iteratee, false,
                                                                        variableToken.position()).id();
                writer.writeCoin(internalId, token);  //Internally needed
                int id = scoper.currentScope().setLocalVariable(variableToken.value(), itemType, true,
                                                                variableToken.position(), true).id();
                valueVariablePlaceholder.write(id);
            }
            else {
                auto iterateeString = iteratee.toString(typeContext, true);
                throw CompilerErrorException(token, "%s does not conform to s🔂.", iterateeString.c_str());
            }
            
            flowControlBlock(false);
            returned = false;
            scoper.popScopeAndRecommendFrozenVariables();

            return Type::nothingness();
        }
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE: {
            throw CompilerErrorException(token, "Unexpected identifier %s.", token.value().utf8().c_str());
            return Type::nothingness();
        }
        case E_DOG: {
            usedSelf = true;
            writer.writeCoin(INS_GET_THIS, token);
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
            writer.writeCoin(INS_GET_NOTHINGNESS, token);
            return Type::nothingness();
        }
        case E_CLOUD: {
            writer.writeCoin(INS_IS_NOTHINGNESS, token);
            parse(stream_.consumeToken());
            return Type::boolean();
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writer.writeCoin(INS_SAME_OBJECT, token);
            
            parse(stream_.consumeToken(), token, Type::someobject());
            parse(stream_.consumeToken(), token, Type::someobject());
            
            return Type::boolean();
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
            
            writer.writeCoin(INS_SUPER_INITIALIZER, token);
            
            Class *eclass = typeContext.calleeType().eclass();
            
            writer.writeCoin(INS_GET_CLASS_FROM_INDEX, token);
            writer.writeCoin(eclass->superclass()->index, token);
            
            auto &initializerToken = stream_.consumeToken(TokenType::Identifier);
            
            auto initializer = eclass->superclass()->getInitializer(initializerToken, Type(eclass), typeContext);
            
            writer.writeCoin(initializer->vtiForUse(), token);
            
            parseFunctionCall(typeContext.calleeType(), initializer, token);

            calledSuper = true;
            
            return Type::nothingness();
        }
        case E_RED_APPLE: {
            if (effect) {
                // Effect is true, so apple is called as subcommand
                throw CompilerErrorException(token, "🍎’s return may not be used as an argument.");
            }
            effect = true;
            
            writer.writeCoin(INS_RETURN, token);
            
            if (mode == CallableParserAndGeneratorMode::ObjectInitializer) {
                if (static_cast<Initializer &>(callable).canReturnNothingness) {
                    parse(stream_.consumeToken(), token, Type::nothingness());
                    return Type::nothingness();
                }
                else {
                    throw CompilerErrorException(token, "🍎 cannot be used inside an initializer.");
                }
            }
            
            parse(stream_.consumeToken(), token, callable.returnType);
            returned = true;
            return Type::nothingness();
        }
        case E_WHITE_LARGE_SQUARE: {
            writer.writeCoin(INS_GET_CLASS_FROM_INSTANCE, token);
            Type originalType = parse(stream_.consumeToken());
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerErrorException(token, "Can’t get metatype of %s.", string.c_str());
            }
            originalType.setMeta(true);
            return originalType;
        }
        case E_WHITE_SQUARE_BUTTON: {
            writer.writeCoin(INS_GET_CLASS_FROM_INDEX, token);
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
                
                placeholder.write(originalType.type() == TypeContent::Something || originalType.optional() ?
                                    INS_SAFE_CAST_TO_CLASS : INS_CAST_TO_CLASS);
            }
            else if (type.type() == TypeContent::Protocol && isStatic(pair.second)) {
                placeholder.write(originalType.type() == TypeContent::Something || originalType.optional() ?
                                    INS_SAFE_CAST_TO_PROTOCOL : INS_CAST_TO_PROTOCOL);
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
            writer.writeCoin(INS_UNWRAP_OPTIONAL, token);
            
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
            if (stream_.consumeTokenIf(E_DOUGHNUT)) {
                auto &methodName = stream_.consumeToken();
                auto pair = parseTypeAsValue(typeContext, token);
                auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);
                
                if (type.type() == TypeContent::Class) {
                    placeholder.write(INS_CAPTURE_TYPE_METHOD);
                }
                else if (type.type() == TypeContent::ValueType) {
                    notStaticError(pair.second, token, "Value Types");
                    placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
                    writer.writeCoin(INS_GET_NOTHINGNESS, token);
                }
                else {
                    throw CompilerErrorException(token, "You can’t capture method calls on this kind of type.");
                }
                
                function = type.eclass()->getTypeMethod(methodName, type, typeContext);
            }
            else {
                auto &methodName = stream_.consumeToken();
                Type type = parse(stream_.consumeToken());
                
                if (type.type() == TypeContent::Class) {
                    placeholder.write(INS_CAPTURE_METHOD);
                }
                else if (type.type() == TypeContent::ValueType) {
                    placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
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
            writer.writeCoin(INS_CLOSURE, token);
            
            auto function = Closure(token.position());
            parseArgumentList(&function, typeContext);
            parseReturnType(&function, typeContext);
            parseBody(&function);
            
            auto variableCountPlaceholder = writer.writeCoinPlaceholder(token);
            auto coinCountPlaceholder = writer.writeCoinsCountPlaceholderCoin(token);
            
            auto closureScoper = CapturingCallableScoper(scoper);

            auto analyzer = CallableParserAndGenerator(function, package_, mode, typeContext, writer, closureScoper);  // TODO: Intializer
            analyzer.analyze();
            
            coinCountPlaceholder.write();
            variableCountPlaceholder.write(closureScoper.fullSize());
            writer.writeCoin(static_cast<EmojicodeCoin>(function.arguments.size())
                             | (analyzer.usedSelfInBody() ? 1 << 16 : 0), token);

            writer.writeCoin(static_cast<EmojicodeCoin>(closureScoper.captures().size()), token);
            writer.writeCoin(closureScoper.captureSize(), token);
            for (auto capture : closureScoper.captures()) {
                writer.writeCoin(capture.id, token);
                writer.writeCoin(capture.type.size(), token);
                writer.writeCoin(capture.captureId, token);
            }

            return function.type();
        }
        case E_LOLLIPOP: {
            writer.writeCoin(INS_EXECUTE_CALLABLE, token);
            
            Type type = parse(stream_.consumeToken());
            
            if (type.type() != TypeContent::Callable) {
                throw CompilerErrorException(token, "Given value is not callable.");
            }

            for (int i = 1; i < type.genericArguments.size(); i++) {
                writer.writeCoin(type.genericArguments[i].size(), token);
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
            
            writer.writeCoin(INS_DISPATCH_SUPER, token);
            writer.writeCoin(INS_GET_CLASS_FROM_INDEX, token);
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
                placeholder.write(INS_GET_32_INTEGER);
                
                auto v = type.eenum()->getValueFor(initializerName.value());
                if (!v.first) {
                    throw CompilerErrorException(initializerName, "%s does not have a member named %s.",
                                                 type.eenum()->name().utf8().c_str(),
                                                 initializerName.value().utf8().c_str());
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
                placeholder.write(INS_CALL_FUNCTION);
                
                auto initializer = type.valueType()->getInitializer(initializerName, type, typeContext);
                writer.writeCoin(initializer->vtiForUse(), token);
                parseFunctionCall(type, initializer, token);
                if (initializer->canReturnNothingness) {
                    type.setOptional();
                }
                return type;
            }
            else if (type.type() == TypeContent::Class) {
                placeholder.write(INS_NEW_OBJECT);
                
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
                placeholder.write(INS_DISPATCH_TYPE_METHOD);
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::ValueType && isStatic(pair.second)) {
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                placeholder.write(INS_CALL_FUNCTION);
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
                    switch (token.value()[0]) {
                        case E_NEGATIVE_SQUARED_CROSS_MARK:
                            placeholder.write(INS_INVERT_BOOLEAN);
                            return Type::boolean();
                        case E_PARTY_POPPER:
                            placeholder.write(INS_OR_BOOLEAN);
                            parse(stream_.consumeToken(), token, Type::boolean());
                            return Type::boolean();
                        case E_CONFETTI_BALL:
                            placeholder.write(INS_AND_BOOLEAN);
                            parse(stream_.consumeToken(), token, Type::boolean());
                            return Type::boolean();
                    }
                }
                else if (type.valueType() == VT_INTEGER) {
                    switch (token.value()[0]) {
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(INS_SUBTRACT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(INS_ADD_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(INS_DIVIDE_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(INS_MULTIPLY_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(INS_LESS_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::boolean();
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(INS_GREATER_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::boolean();
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(INS_LESS_OR_EQUAL_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::boolean();
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(INS_GREATER_OR_EQUAL_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::boolean();
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(INS_REMAINDER_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_HEAVY_LARGE_CIRCLE:
                            placeholder.write(INS_BINARY_AND_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_ANGER_SYMBOL:
                            placeholder.write(INS_BINARY_OR_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_CROSS_MARK:
                            placeholder.write(INS_BINARY_XOR_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_NO_ENTRY_SIGN:
                            placeholder.write(INS_BINARY_NOT_INTEGER);
                            return Type::integer();
                        case E_ROCKET:
                            placeholder.write(INS_INT_TO_DOUBLE);
                            return Type::doubl();
                        case E_LEFT_POINTING_BACKHAND_INDEX:
                            placeholder.write(INS_SHIFT_LEFT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                        case E_RIGHT_POINTING_BACKHAND_INDEX:
                            placeholder.write(INS_SHIFT_RIGHT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer());
                            return Type::integer();
                    }
                }
                else if (type.valueType() == VT_DOUBLE) {
                    switch (token.value()[0]) {
                        case E_FACE_WITH_STUCK_OUT_TONGUE:
                            placeholder.write(INS_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::boolean();
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(INS_SUBTRACT_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::doubl();
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(INS_ADD_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::doubl();
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(INS_DIVIDE_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::doubl();
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(INS_MULTIPLY_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::doubl();
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(INS_LESS_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::boolean();
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(INS_GREATER_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::boolean();
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(INS_LESS_OR_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::boolean();
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(INS_GREATER_OR_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::boolean();
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(INS_REMAINDER_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl());
                            return Type::doubl();
                    }
                }
                
                if (token.isIdentifier(E_FACE_WITH_STUCK_OUT_TONGUE)) {
                    parse(stream_.consumeToken(), token, type);  // Must be of the same type as the callee
                    placeholder.write(INS_EQUAL_PRIMITIVE);
                    return Type::boolean();
                }
                
                method = type.valueType()->getMethod(token, type, typeContext);
                placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Protocol) {
                method = type.protocol()->getMethod(token, type, typeContext);
                placeholder.write(INS_DISPATCH_PROTOCOL);
                writer.writeCoin(type.protocol()->index, token);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Enum && token.value()[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
                parse(stream_.consumeToken(), token, type);  // Must be of the same type as the callee
                placeholder.write(INS_EQUAL_PRIMITIVE);
                return Type::boolean();
            }
            else if (type.type() == TypeContent::Class) {
                method = type.eclass()->getMethod(token, type, typeContext);
                placeholder.write(INS_DISPATCH_METHOD);
                writer.writeCoin(method->vtiForUse(), token);
            }
            else {
                auto typeString = type.toString(typeContext, true);
                throw CompilerErrorException(token, "You cannot call methods on %s.", typeString.c_str());
            }
            
            return parseFunctionCall(type, method, token);
        }
    }
    return Type::nothingness();
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
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
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
            writer.writeCoin(INS_GET_CLASS_FROM_INDEX, p);
            writer.writeCoin(ot.eclass()->index, p);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndAvailabale);
        case TypeContent::Self:
            if (mode != CallableParserAndGeneratorMode::ClassMethod) {
                throw CompilerErrorException(p, "Illegal use of 🐕.");
            }
            writer.writeCoin(INS_GET_THIS, p);
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

void CallableParserAndGenerator::analyze() {
    auto isClassInitializer = mode == CallableParserAndGeneratorMode::ObjectInitializer;
    if (mode == CallableParserAndGeneratorMode::ObjectMethod ||
        mode == CallableParserAndGeneratorMode::ObjectInitializer) {
        scoper.objectScope()->setVariableInitialization(!isClassInitializer);
    }
    try {
        Scope &methodScope = scoper.pushScope();
        for (auto variable : callable.arguments) {
            methodScope.setLocalVariable(variable.name.value(), variable.type, true, variable.name.position(), true);
        }
        
        if (isClassInitializer) {
            auto initializer = static_cast<Initializer &>(callable);
            for (auto &var : initializer.argumentsToVariables()) {
                if (scoper.objectScope()->hasLocalVariable(var) == 0) {
                    throw CompilerErrorException(initializer.position(),
                                                 "🍼 was applied to \"%s\" but no matching instance variable was found.",
                                                 var.utf8().c_str());
                }
                auto &instanceVariable = scoper.objectScope()->getLocalVariable(var);
                auto &argumentVariable = methodScope.getLocalVariable(var);
                if (!argumentVariable.type.compatibleTo(instanceVariable.type, typeContext)) {
                    throw CompilerErrorException(initializer.position(),
                                                 "🍼 was applied to \"%s\" but instance variable has incompatible type.",
                                                 var.utf8().c_str());
                }
                instanceVariable.initialized = 1;
                writer.writeCoin(INS_PRODUCE_WITH_INSTANCE_DESTINATION, initializer.position());
                writer.writeCoin(instanceVariable.id(), initializer.position());
                copyVariableContent(argumentVariable, false, initializer.position());
            }
        }

        while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
            parseCoinInBlock();
           
            if (returned && !stream_.nextTokenIs(E_WATERMELON)) {
                compilerWarning(stream_.consumeToken(), "Dead code.");
                break;
            }
        }
        
        scoper.popScopeAndRecommendFrozenVariables();
        
        if (isClassInitializer) {
            auto initializer = static_cast<Initializer &>(callable);
            scoper.objectScope()->initializerUnintializedVariablesCheck(initializer.position(),
                                                                    "Instance variable \"%s\" must be initialized.");
            
            if (!calledSuper && typeContext.calleeType().eclass()->superclass()) {
                throw CompilerErrorException(initializer.position(),
                              "Missing call to superinitializer in initializer %s.", initializer.name().utf8().c_str());
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
    function->setFullSize(scoper.fullSize());
}
