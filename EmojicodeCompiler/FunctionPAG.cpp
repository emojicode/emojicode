//
//  FunctionPAG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "FunctionPAG.hpp"
#include "Type.hpp"
#include "Class.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "Function.hpp"
#include "ValueType.hpp"
#include "CommonTypeFinder.hpp"
#include "VariableNotFoundError.hpp"
#include "StringPool.hpp"
#include "../EmojicodeInstructions.h"
#include "CapturingCallableScoper.hpp"
#include "RecompilationPoint.hpp"
#include <cstdlib>
#include <cassert>

Type FunctionPAG::parse(const Token &token, const Token &parentToken, Type type, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = ctargs ? parse(token, TypeExpectation(type.isReference(), type.requiresBox(), false)) : parse(token, TypeExpectation(type));
    if (!returnType.compatibleTo(type, typeContext, ctargs)) {
        auto cn = returnType.toString(typeContext, true);
        auto tn = type.toString(typeContext, true);
        throw CompilerError(token.position(), "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

bool FunctionPAG::typeIsEnumerable(Type type, Type *elementType) {
    if (type.type() == TypeContent::Class && !type.optional()) {
        for (Class *a = type.eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.protocol() == PR_ENUMERATEABLE) {
                    *elementType = protocol.resolveOn(type).genericArguments()[0];
                    return true;
                }
            }
        }
    }
    else if (type.type() == TypeContent::Protocol && type.protocol() == PR_ENUMERATEABLE) {
        *elementType = Type(TypeContent::Reference, false, 0, type.protocol()).resolveOn(type);
        return true;
    }
    return false;
}

void FunctionPAG::checkAccessLevel(Function *function, const SourcePosition &p) const {
    if (function->accessLevel() == AccessLevel::Private) {
        if (typeContext.calleeType().type() != function->owningType().type()
            || function->owningType().typeDefinition() != typeContext.calleeType().typeDefinition()) {
            throw CompilerError(p, "%s is 🔒.", function->name().utf8().c_str());
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (typeContext.calleeType().type() != function->owningType().type()
            || !this->typeContext.calleeType().eclass()->inheritsFrom(function->owningType().eclass())) {
            throw CompilerError(p, "%s is 🔐.", function->name().utf8().c_str());
        }
    }
}

Type FunctionPAG::parseFunctionCall(Type type, Function *p, const Token &token) {
    std::vector<Type> genericArguments;
    std::vector<CommonTypeFinder> genericArgsFinders;
    std::vector<Type> givenArgumentTypes;

    p->deprecatedWarning(token.position());

    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        auto type = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);
        genericArguments.push_back(type);
    }

    auto inferGenericArguments = genericArguments.empty() && !p->genericArgumentVariables.empty();
    auto typeContext = TypeContext(type, p, inferGenericArguments ? nullptr : &genericArguments);

    if (inferGenericArguments) {
        genericArgsFinders.resize(p->genericArgumentVariables.size());
    }
    else if (genericArguments.size() != p->genericArgumentVariables.size()) {
        throw CompilerError(token.position(), "Too few generic arguments provided.");
    }
    else {
        for (size_t i = 0; i < genericArguments.size(); i++) {
            if (!genericArguments[i].compatibleTo(p->genericArgumentConstraints[i], typeContext)) {
                throw CompilerError(token.position(),
                                    "Generic argument %d of type %s is not compatible to constraint %s.",
                                    i + 1, genericArguments[i].toString(typeContext, true).c_str(),
                                    p->genericArgumentConstraints[i].toString(typeContext, true).c_str());
            }
        }
    }

    for (auto var : p->arguments) {
        auto resolved = var.type.resolveOn(typeContext);
        if (inferGenericArguments) {
            writer.writeInstruction(resolved.size());
            givenArgumentTypes.push_back(parse(stream_.consumeToken(), token, resolved, &genericArgsFinders));
        }
        else {
            writer.writeInstruction(resolved.size());
            parse(stream_.consumeToken(), token, resolved);
        }
    }

    if (inferGenericArguments) {
        auto typeContext = TypeContext(type, p, &genericArguments);
        for (size_t i = 0; i < genericArgsFinders.size(); i++) {
            auto commonType = genericArgsFinders[i].getCommonType(token.position());
            genericArguments.push_back(commonType);
            if (!commonType.compatibleTo(p->genericArgumentConstraints[i], typeContext)) {
                throw CompilerError(token.position(),
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
                throw CompilerError(token.position(), "%s is not compatible to %s.", cn.c_str(), tn.c_str());
            }
        }
    }
    checkAccessLevel(p, token.position());
    return p->returnType.resolveOn(typeContext);
}

void FunctionPAG::writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                                                    EmojicodeInstruction object,
                                                                    EmojicodeInstruction vt) {
    if (!inInstanceScope) {
        writer.writeInstruction(stack);
    }
    else {
        writer.writeInstruction(typeContext.calleeType().type() == TypeContent::ValueType ? vt : object);
        usedSelf = true;
    }
}

void FunctionPAG::produceToVariable(ResolvedVariable var) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_PRODUCE_WITH_STACK_DESTINATION,
                                       INS_PRODUCE_WITH_OBJECT_DESTINATION, INS_PRODUCE_WITH_VT_DESTINATION);
    writer.writeInstruction(var.variable.id());
}

void FunctionPAG::getVTReference(ResolvedVariable var) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_GET_VT_REFERENCE_STACK, INS_GET_VT_REFERENCE_OBJECT,
                                       INS_GET_VT_REFERENCE_VT);
    writer.writeInstruction(var.variable.id());
}

void FunctionPAG::copyVariableContent(ResolvedVariable var) {
    if (var.variable.type().size() == 1) {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_SINGLE_STACK, INS_COPY_SINGLE_OBJECT,
                                           INS_COPY_SINGLE_VT);
        writer.writeInstruction(var.variable.id());
    }
    else {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_WITH_SIZE_STACK, INS_COPY_WITH_SIZE_OBJECT,
                                           INS_COPY_WITH_SIZE_VT);
        writer.writeInstruction(var.variable.id());
        writer.writeInstruction(var.variable.type().size());
    }
}

void FunctionPAG::flowControlBlock(bool pushScope, const std::function<void()> &bodyPredicate) {
    scoper.pushInitializationLevel();

    if (pushScope) {
        scoper.pushScope();
    }

    flowControlDepth++;

    stream_.requireIdentifier(E_GRAPES);

    auto placeholder = writer.writeInstructionsCountPlaceholderCoin();
    if (bodyPredicate) {
        bodyPredicate();
    }
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        parseStatement(stream_.consumeToken());
    }
    stream_.consumeToken();
    placeholder.write();

    effect = true;

    scoper.popScopeAndRecommendFrozenVariables(callable.objectVariableInformation(), writer.writtenInstructions());

    scoper.popInitializationLevel();
    flowControlDepth--;
}

void FunctionPAG::flowControlReturnEnd(FlowControlReturn &fcr) {
    if (returned) {
        fcr.branchReturns++;
        returned = false;
    }
    fcr.branches++;
}

void FunctionPAG::writeBoxingAndTemporary(const TypeExpectation &expectation, Type &rtype, WriteLocation location) const {
    if (rtype.type() == TypeContent::Nothingness) {
        return;
    }
    if (!expectation.shouldPerformBoxing()) {
        return;
    }

    auto insertionPoint = location.insertionPoint();

    switch (expectation.simplifyType(rtype)) {
        case StorageType::SimpleOptional:
            switch (rtype.storageType()) {
                case StorageType::SimpleOptional:
                    break;
                case StorageType::Box:
                    rtype.unbox();
                    rtype.setOptional();
                    location.write({ INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE, EmojicodeInstruction(rtype.size()) });
                    break;
                case StorageType::Simple:
                    location.write({ INS_SIMPLE_OPTIONAL_PRODUCE });
                    break;
            }
            break;
        case StorageType::Box:
            switch (rtype.storageType()) {
                case StorageType::Box:
                    break;
                case StorageType::SimpleOptional:
                    if ((rtype.size() > 3 && !rtype.optional()) || rtype.size() > 4) {
                        throw std::logic_error("Beta: Cannot box " + rtype.toString(typeContext, true) + " due to its size.");
                    }
                    rtype.forceBox();
                    location.write({ INS_SIMPLE_OPTIONAL_TO_BOX, EmojicodeInstruction(rtype.boxIdentifier()) });
                    break;
                case StorageType::Simple:
                    if ((rtype.size() > 3 && !rtype.optional()) || rtype.size() > 4) {
                        throw std::logic_error("Beta: Cannot box " + rtype.toString(typeContext, true) + " due to its size.");
                    }
                    rtype.forceBox();
                    location.write({ INS_BOX_PRODUCE, EmojicodeInstruction(rtype.boxIdentifier()) });
                    break;
            }
            break;
        case StorageType::Simple:
            switch (rtype.storageType()) {
                case StorageType::Simple:
                case StorageType::SimpleOptional:
                    break;
                case StorageType::Box:
                    rtype.unbox();
                    location.write({ INS_UNBOX, EmojicodeInstruction(rtype.size()) });
                    break;
            }
            break;
    }

    if (!rtype.isReference() && expectation.isReference() && rtype.isReferenceWorthy()) {
        scoper.pushTemporaryScope();
        int destinationID = scoper.currentScope().allocateInternalVariable(rtype);
        insertionPoint.insert({ INS_PRODUCE_TO_AND_GET_VT_REFERENCE, EmojicodeInstruction(destinationID) });
        rtype.setReference();
    }
    else if (rtype.isReference() && !expectation.isReference()) {
        rtype.setReference(false);
        insertionPoint.insert({ INS_COPY_REFERENCE, EmojicodeInstruction(rtype.size()) });
    }
}

void FunctionPAG::parseIfExpression(const Token &token) {
    if (stream_.consumeTokenIf(E_SOFT_ICE_CREAM)) {
        auto placeholder = writer.writeInstructionPlaceholder();

        auto &varName = stream_.consumeToken(TokenType::Variable);
        if (scoper.currentScope().hasLocalVariable(varName.value())) {
            throw CompilerError(token.position(), "Cannot redeclare variable.");
        }

        Type t = parse(stream_.consumeToken(), TypeExpectation(true, false));
        if (!t.optional()) {
            throw CompilerError(token.position(), "Condition assignment can only be used with optionals.");
        }

        auto box = t.storageType() == StorageType::Box;
        placeholder.write(box ? INS_CONDITIONAL_PRODUCE_BOX : INS_CONDITIONAL_PRODUCE_SIMPLE_OPTIONAL);

        t.setReference(false);
        t = t.copyWithoutOptional();

        auto &variable = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
        variable.initialize(writer.writtenInstructions());
        writer.writeInstruction(variable.id());
        if (!box) {
            writer.writeInstruction(t.size());
        }
    }
    else {
        parse(stream_.consumeToken(TokenType::NoType), token, Type::boolean());
    }
}

bool FunctionPAG::isSuperconstructorRequired() const {
    return mode == FunctionPAGMode::ObjectInitializer;
}

bool FunctionPAG::isFullyInitializedCheckRequired() const {
    return mode == FunctionPAGMode::ObjectInitializer ||
    mode == FunctionPAGMode::ValueTypeInitializer;
}

bool FunctionPAG::isSelfAllowed() const {
    return mode != FunctionPAGMode::Function &&
    mode != FunctionPAGMode::ClassMethod;
}

bool FunctionPAG::hasInstanceScope() const {
    return hasInstanceScope(mode);
}

bool FunctionPAG::hasInstanceScope(FunctionPAGMode mode) {
    return (mode == FunctionPAGMode::ObjectMethod ||
            mode == FunctionPAGMode::ObjectInitializer ||
            mode == FunctionPAGMode::ValueTypeInitializer ||
            mode == FunctionPAGMode::ValueTypeMethod);
}

bool FunctionPAG::isOnlyNothingnessReturnAllowed() const {
    return mode == FunctionPAGMode::ObjectInitializer ||
    mode == FunctionPAGMode::ValueTypeInitializer;
}

void FunctionPAG::mutatingMethodCheck(Function *method, Type type, const TypeExpectation &expectation,
                                      const SourcePosition &p) {
    if (method->mutating()) {
        if (!type.isMutable()) {
            throw CompilerError(p, "%s was marked 🖍 but callee is not mutable.", method->name().utf8().c_str());
        }
        expectation.mutateOriginVariable(p);
    }
}

void FunctionPAG::parseStatement(const Token &token) {
    if (token.type() == TokenType::Identifier) {
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                Type t = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);

                auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, false, varName.position());

                if (t.optional()) {
                    var.initialize(writer.writtenInstructions());
                    writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);
                    writer.writeInstruction(var.id());
                    writer.writeInstruction(INS_GET_NOTHINGNESS);
                }
                return;
            }
            case E_CUSTARD: {
                auto &nextTok = stream_.consumeToken();

                if (nextTok.type() == TokenType::Identifier) {
                    auto &varName = stream_.consumeToken(TokenType::Variable);

                    auto var = scoper.getVariable(varName.value(), varName.position());

                    var.variable.uninitalizedError(varName.position());
                    var.variable.mutate(varName.position());

                    produceToVariable(var);

                    if (var.variable.type().type() == TypeContent::ValueType &&
                        var.variable.type().valueType() == VT_INTEGER && stream_.nextTokenIs(TokenType::Integer)
                        && stream_.nextToken().value() == EmojicodeString('1')) {
                        if (nextTok.value()[0] == E_HEAVY_PLUS_SIGN) {
                            writer.writeInstruction(INS_INCREMENT);
                            stream_.consumeToken();
                            return;
                        }
                        if (nextTok.value()[0] == E_HEAVY_MINUS_SIGN) {
                            writer.writeInstruction(INS_DECREMENT);
                            stream_.consumeToken();
                            return;
                        }
                    }

                    auto type = parseMethodCall(nextTok, TypeExpectation(var.variable.type()),
                                                [this, var, &nextTok](const TypeExpectation &expectation) {
                        takeVariable(var, expectation);
                        return var.variable.type();
                    });
                    if (!type.compatibleTo(var.variable.type(), typeContext)) {
                        throw CompilerError(nextTok.position(), "Method %s returns incompatible type %s.",
                                            nextTok.value().utf8().c_str(), type.toString(typeContext, true).c_str());
                    }
                }
                else {
                    Type type = Type::nothingness();
                    try {
                        auto rvar = scoper.getVariable(nextTok.value(), nextTok.position());

                        rvar.variable.initialize(writer.writtenInstructions());
                        rvar.variable.mutate(nextTok.position());

                        produceToVariable(rvar);

                        if (rvar.inInstanceScope && !typeContext.calleeType().isMutable()) {
                            throw CompilerError(token.position(),
                                                "Can’t mutate instance variable as method is not marked with 🖍.");
                        }

                        type = rvar.variable.type();
                    }
                    catch (VariableNotFoundError &vne) {
                        // Not declared, declaring as local variable
                        writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);

                        auto placeholder = writer.writeInstructionPlaceholder();

                        Type t = parse(stream_.consumeToken(), TypeExpectation(false, true));
                        auto &var = scoper.currentScope().setLocalVariable(nextTok.value(), t, false, nextTok.position());
                        var.initialize(writer.writtenInstructions());
                        placeholder.write(var.id());
                        return;
                    }
                    parse(stream_.consumeToken(), token, type);
                }
                return;
            }
            case E_SOFT_ICE_CREAM: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);

                auto placeholder = writer.writeInstructionPlaceholder();

                Type t = parse(stream_.consumeToken(), TypeExpectation(false, false));
                auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
                var.initialize(writer.writtenInstructions());
                placeholder.write(var.id());
                return;
            }
            case E_TANGERINE: {
                writer.writeInstruction(INS_IF);

                auto placeholder = writer.writeInstructionsCountPlaceholderCoin();
                auto fcr = FlowControlReturn();

                scoper.pushScope();
                parseIfExpression(token);
                flowControlBlock(false);
                flowControlReturnEnd(fcr);

                while (stream_.consumeTokenIf(E_LEMON)) {
                    writer.writeInstruction(0x1);

                    scoper.pushScope();
                    parseIfExpression(token);
                    flowControlBlock(false);
                    flowControlReturnEnd(fcr);
                }

                if (stream_.consumeTokenIf(E_STRAWBERRY)) {
                    writer.writeInstruction(0x2);
                    flowControlBlock();
                    flowControlReturnEnd(fcr);
                }
                else {
                    writer.writeInstruction(0x0);
                    fcr.branches++;  // The else branch always exists. Theoretically at least.
                }

                placeholder.write();

                returned = fcr.returned();

                return;
            }
            case E_AVOCADO: {
                auto &variableToken = stream_.consumeToken(TokenType::Variable);

                writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);
                auto placeholder = writer.writeInstructionPlaceholder();

                Type type = parse(stream_.consumeToken(), TypeExpectation(false, false));

                if (type.type() != TypeContent::Error) {
                    throw CompilerError(token.position(), "🥑 can only be used with 🚨.");
                }

                auto &wrscope = scoper.pushScope();
                int variableID = wrscope.allocateInternalVariable(type);
                placeholder.write(variableID);

                auto &scope = scoper.pushScope();

                if (type.storageType() == StorageType::Box) {
                    Type vtype = type.genericArguments()[1];
                    vtype.forceBox();
                    scope.setLocalVariableWithID(variableToken.value(), vtype, true, variableID,
                                                 variableToken.position()).initialize(writer.writtenInstructions());

                }
                else if (type.genericArguments()[1].storageType() == StorageType::SimpleOptional) {
                    scope.setLocalVariableWithID(variableToken.value(), type.genericArguments()[1], true, variableID,
                                                 variableToken.position()).initialize(writer.writtenInstructions());
                }
                else {
                    scope.setLocalVariableWithID(variableToken.value(), type.genericArguments()[1], true,
                                                 variableID + 1, variableToken.position())
                                                 .initialize(writer.writtenInstructions());
                }

                writer.writeInstruction(INS_IF);
                auto countPlaceholder = writer.writeInstructionsCountPlaceholderCoin();
                writer.writeInstruction(INS_INVERT_BOOLEAN);
                writer.writeInstruction(INS_IS_ERROR);
                writer.writeInstruction(INS_GET_VT_REFERENCE_STACK);
                writer.writeInstruction(variableID);
                flowControlBlock(false);
                writer.writeInstruction(0x2);

                stream_.requireIdentifier(E_STRAWBERRY);
                auto &errorVariableToken = stream_.consumeToken(TokenType::Variable);

                auto &errorScope = scoper.pushScope();
                errorScope.setLocalVariableWithID(errorVariableToken.value(), type.genericArguments()[0], true,
                                                  variableID + 1, errorVariableToken.position())
                                                  .initialize(writer.writtenInstructions());
                flowControlBlock(false);

                scoper.popScopeAndRecommendFrozenVariables(callable.objectVariableInformation(),
                                                           writer.writtenInstructions());
                countPlaceholder.write();
                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
                writer.writeInstruction(INS_REPEAT_WHILE);

                parseIfExpression(token);
                flowControlBlock();
                returned = false;

                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
                scoper.pushScope();

                auto &variableToken = stream_.consumeToken(TokenType::Variable);

                auto insertionPoint = writer.getInsertionPoint();
                Type iteratee = parse(stream_.consumeToken(), TypeExpectation());

                Type itemType = Type::nothingness();

                if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_LIST) {
                    // If the iteratee is a list, the Real-Time Engine has some special sugar
                    int internalId = scoper.currentScope().allocateInternalVariable(iteratee);
                    writer.writeInstruction(internalId);  // Internally needed
                    auto type = Type(TypeContent::Reference, false, 0, CL_LIST).resolveOn(iteratee);
                    auto &var = scoper.currentScope().setLocalVariable(variableToken.value(), type,
                                                                       true, variableToken.position());
                    var.initialize(writer.writtenInstructions());
                    writeBoxingAndTemporary(TypeExpectation(true, false), iteratee, insertionPoint);
                    insertionPoint.insert({ INS_OPT_FOR_IN_LIST, static_cast<unsigned int>(var.id()) });
                    flowControlBlock(false);
                }
                else if (iteratee.type() == TypeContent::ValueType &&
                         iteratee.valueType()->name()[0] == E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE) {
                    // If the iteratee is a range, the Real-Time Engine also has some special sugar
                    auto &var = scoper.currentScope().setLocalVariable(variableToken.value(), Type::integer(), true,
                                                                       variableToken.position());
                    var.initialize(writer.writtenInstructions());
                    writeBoxingAndTemporary(TypeExpectation(true, false), iteratee, insertionPoint);
                    insertionPoint.insert({ INS_OPT_FOR_IN_RANGE, static_cast<unsigned int>(var.id()) });
                    flowControlBlock(false);
                }
                else if (typeIsEnumerable(iteratee, &itemType)) {
                    auto iteratorMethodIndex = PR_ENUMERATEABLE->lookupMethod(EmojicodeString(E_DANGO))->vtiForUse();

                    auto iteratorId = scoper.currentScope().allocateInternalVariable(Type(PR_ENUMERATOR, false));
                    auto nextVTI = PR_ENUMERATOR->lookupMethod(EmojicodeString(E_DOWN_POINTING_SMALL_RED_TRIANGLE))->vtiForUse();
                    auto moreVTI = PR_ENUMERATOR->lookupMethod(EmojicodeString(E_RED_QUESTION_MARK))->vtiForUse();

                    auto &var = scoper.currentScope().setLocalVariable(variableToken.value(), itemType, true,
                                                                       variableToken.position());
                    var.initialize(writer.writtenInstructions());

                    writeBoxingAndTemporary(TypeExpectation(true, true, false), iteratee, insertionPoint);
                    insertionPoint.insert({ INS_PRODUCE_WITH_STACK_DESTINATION,
                        static_cast<EmojicodeInstruction>(iteratorId), INS_DISPATCH_PROTOCOL,
                    });
                    writer.writeInstruction({ static_cast<EmojicodeInstruction>(PR_ENUMERATEABLE->index),
                        static_cast<EmojicodeInstruction>(iteratorMethodIndex), INS_REPEAT_WHILE, INS_DISPATCH_PROTOCOL,
                        INS_GET_VT_REFERENCE_STACK, static_cast<EmojicodeInstruction>(iteratorId),
                        static_cast<EmojicodeInstruction>(PR_ENUMERATOR->index),
                        static_cast<EmojicodeInstruction>(moreVTI)
                    });
                    flowControlBlock(false, [this, iteratorId, &var, nextVTI]{
                        writer.writeInstruction({ INS_PRODUCE_WITH_STACK_DESTINATION,
                            static_cast<EmojicodeInstruction>(var.id()), INS_DISPATCH_PROTOCOL, INS_GET_VT_REFERENCE_STACK,
                            static_cast<EmojicodeInstruction>(iteratorId),
                            static_cast<EmojicodeInstruction>(PR_ENUMERATOR->index),
                            static_cast<EmojicodeInstruction>(nextVTI)
                        });
                    });
                }
                else {
                    auto iterateeString = iteratee.toString(typeContext, true);
                    throw CompilerError(token.position(), "%s does not conform to s🔂.", iterateeString.c_str());
                }

                returned = false;
                return;
            }
            case E_GOAT: {
                if (!isSuperconstructorRequired()) {
                    throw CompilerError(token.position(), "🐐 can only be used inside initializers.");
                }
                if (typeContext.calleeType().eclass()->superclass() == nullptr) {
                    throw CompilerError(token.position(), "🐐 can only be used if the eclass inherits from another.");
                }
                if (calledSuper) {
                    throw CompilerError(token.position(), "You may not call more than one superinitializer.");
                }
                if (flowControlDepth > 0) {
                    throw CompilerError(token.position(),
                                        "You may not put a call to a superinitializer in a flow control structure.");
                }

                scoper.instanceScope()->initializerUnintializedVariablesCheck(token.position(),
                                                                              "Instance variable \"%s\" must be "
                                                                              "initialized before superinitializer.");

                writer.writeInstruction(INS_SUPER_INITIALIZER);

                Class *eclass = typeContext.calleeType().eclass();

                writer.writeInstruction(INS_GET_CLASS_FROM_INDEX);
                writer.writeInstruction(eclass->superclass()->index);

                auto &initializerToken = stream_.consumeToken(TokenType::Identifier);

                auto initializer = eclass->superclass()->getInitializer(initializerToken, Type(eclass, false), typeContext);

                writer.writeInstruction(initializer->vtiForUse());

                parseFunctionCall(typeContext.calleeType(), initializer, token);

                calledSuper = true;

                return;
            }
            case E_POLICE_CARS_LIGHT: {
                writer.writeInstruction(INS_ERROR);

                if (isOnlyNothingnessReturnAllowed()) {
                    auto &initializer = static_cast<Initializer &>(callable);
                    if (!initializer.errorProne()) {
                        throw CompilerError(token.position(), "Initializer is not declared error-prone.");
                    }
                    parse(stream_.consumeToken(), token, initializer.errorType());
                    returned = true;
                    return;
                }
                if (callable.returnType.type() != TypeContent::Error) {
                    throw CompilerError(token.position(), "Function is not declared to return a 🚨.");
                }

                parse(stream_.consumeToken(), token, callable.returnType.genericArguments()[0]);
                returned = true;
                return;

            }
            case E_RED_APPLE: {
                writer.writeInstruction(INS_RETURN);

                if (isOnlyNothingnessReturnAllowed()) {
                    throw CompilerError(token.position(), "🍎 cannot be used inside an initializer.");
                }

                parse(stream_.consumeToken(), token, callable.returnType);
                returned = true;
                return;
            }
        }
    }

    // Must be an expression
    effect = false;
    auto type = parse(token, TypeExpectation(true, false, false));
    noEffectWarning(token);
    scoper.clearAllTemporaryScopes();
}

Type FunctionPAG::parse(const Token &token, TypeExpectation &&expectation) {
    switch (token.type()) {
        case TokenType::String: {
            writeBoxingAndTemporary(expectation, Type(CL_STRING, false));
            writer.writeInstruction(INS_GET_STRING_POOL);
            writer.writeInstruction(StringPool::theStringPool().poolString(token.value()));
            return Type(CL_STRING, false);
        }
        case TokenType::BooleanTrue:
            writeBoxingAndTemporary(expectation, Type::boolean());
            writer.writeInstruction(INS_GET_TRUE);
            return Type::boolean();
        case TokenType::BooleanFalse:
            writeBoxingAndTemporary(expectation, Type::boolean());
            writer.writeInstruction(INS_GET_FALSE);
            return Type::boolean();
        case TokenType::Integer: {
            long long value = std::stoll(token.value().utf8(), nullptr, 0);

            if (expectation.type() == TypeContent::ValueType && expectation.valueType() == VT_DOUBLE) {
                writeBoxingAndTemporary(expectation, Type::doubl());
                writer.writeInstruction(INS_GET_DOUBLE);
                writer.writeDoubleCoin(value);
                return Type::doubl();
            }

            if (std::llabs(value) > INT32_MAX) {
                writeBoxingAndTemporary(expectation, Type::integer());
                writer.writeInstruction(INS_GET_64_INTEGER);

                writer.writeInstruction(value >> 32);
                writer.writeInstruction(static_cast<EmojicodeInstruction>(value));

                return Type::integer();
            }

            writeBoxingAndTemporary(expectation, Type::integer());
            writer.writeInstruction(INS_GET_32_INTEGER);
            value += INT32_MAX;
            writer.writeInstruction(value);

            return Type::integer();
        }
        case TokenType::Double: {
            writeBoxingAndTemporary(expectation, Type::doubl());
            writer.writeInstruction(INS_GET_DOUBLE);

            double d = std::stod(token.value().utf8());
            writer.writeDoubleCoin(d);

            return Type::doubl();
        }
        case TokenType::Symbol:
            writeBoxingAndTemporary(expectation, Type::symbol());
            writer.writeInstruction(INS_GET_SYMBOL);
            writer.writeInstruction(token.value()[0]);
            return Type::symbol();
        case TokenType::Variable: {
            auto rvar = scoper.getVariable(token.value(), token.position());
            rvar.variable.uninitalizedError(token.position());

            expectation.setOriginVariable(rvar.variable);
            return takeVariable(rvar, expectation);
        }
        case TokenType::Identifier:
            return parseIdentifier(token, expectation);
        case TokenType::DocumentationComment:
            throw CompilerError(token.position(), "Misplaced documentation comment.");
        case TokenType::NoType:
        case TokenType::Comment:
            break;
    }
    throw std::logic_error("Cannot determine expression’s return type.");
}

Type FunctionPAG::takeVariable(ResolvedVariable rvar, const TypeExpectation &expectation) {
    auto storageType = expectation.simplifyType(rvar.variable.type());
    auto returnType = rvar.variable.type();

    if (storageType == rvar.variable.type().storageType() &&
        expectation.isReference() && rvar.variable.type().isReferenceWorthy()) {
        getVTReference(rvar);
        returnType.setReference();
    }
    else {
        writeBoxingAndTemporary(expectation, returnType, writer);
        copyVariableContent(rvar);
        if (expectation.isMutable()) {
            returnType.setMutable(true);
        }
    }
    return returnType;
}

Type FunctionPAG::parseIdentifier(const Token &token, const TypeExpectation &expectation) {
    switch (token.value()[0]) {
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_SHORTCAKE:
        case E_CUSTARD:
        case E_SOFT_ICE_CREAM:
        case E_TANGERINE:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_GOAT:
        case E_RED_APPLE:
        case E_POLICE_CARS_LIGHT:
        case E_AVOCADO:
            throw CompilerError(token.position(), "Unexpected statement %s.", token.value().utf8().c_str());
        case E_COOKIE: {
            writeBoxingAndTemporary(expectation, Type(CL_STRING, false));
            writer.writeInstruction(INS_OPT_STRING_CONCATENATE_LITERAL);
            auto placeholder = writer.writeInstructionPlaceholder();

            int stringCount = 0;

            while (stream_.nextTokenIsEverythingBut(E_COOKIE)) {
                parse(stream_.consumeToken(), token, TypeExpectation(Type(CL_STRING, false)));
                stringCount++;
            }
            stream_.consumeToken(TokenType::Identifier);

            if (stringCount == 0) {
                throw CompilerError(token.position(), "An empty 🍪 is invalid.");
            }

            placeholder.write(stringCount);
            return Type(CL_STRING, false);
        }
        case E_ICE_CREAM: {
            writeBoxingAndTemporary(expectation, Type(CL_LIST, false));
            writer.writeInstruction(INS_OPT_LIST_LITERAL);

            auto placeholder = writer.writeInstructionsCountPlaceholderCoin();

            Type type = Type(CL_LIST, false);
            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_LIST) {
                auto elementType = Type(TypeContent::Reference, false, 0, expectation.eclass()).resolveOn(expectation);
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, TypeExpectation(elementType));
                }
                stream_.consumeToken(TokenType::Identifier);
                type.setGenericArgument(0, elementType);
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    ct.addType(parse(stream_.consumeToken(), TypeExpectation(false, true, false)), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.setGenericArgument(0, ct.getCommonType(token.position()));
            }

            placeholder.write();
            return type;
        }
        case E_HONEY_POT: {
            writeBoxingAndTemporary(expectation, Type(CL_DICTIONARY, false));
            writer.writeInstruction(INS_OPT_DICTIONARY_LITERAL);

            auto placeholder = writer.writeInstructionsCountPlaceholderCoin();
            Type type = Type(CL_DICTIONARY, false);

            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_DICTIONARY) {
                auto elementType = Type(TypeContent::Reference, false, 0, expectation.eclass()).resolveOn(expectation);
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, TypeExpectation(Type(CL_STRING, false)));
                    parse(stream_.consumeToken(), token, TypeExpectation(elementType));
                }
                stream_.consumeToken(TokenType::Identifier);
                type.setGenericArgument(0, elementType);
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    parse(stream_.consumeToken(), token, TypeExpectation(Type(CL_STRING, false)));
                    ct.addType(parse(stream_.consumeToken(), TypeExpectation(false, true, false)), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.setGenericArgument(0, ct.getCommonType(token.position()));
            }

            placeholder.write();
            return type;
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            Type type = Type::nothingness();
            package_->fetchRawType(EmojicodeString(E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE), globalNamespace,
                                   false, token.position(), &type);

            writeBoxingAndTemporary(expectation, type, writer);

            writer.writeInstruction(INS_INIT_VT);
            auto initializer = type.valueType()->getInitializer(token, type, typeContext);
            writer.writeInstruction(initializer->vtiForUse());

            parseFunctionCall(type, initializer, token);

            return type;
        }
        case E_DOG: {
            if (isSuperconstructorRequired() && !calledSuper &&
                callable.owningType().eclass()->superclass() != nullptr) {
                throw CompilerError(token.position(), "Attempt to use 🐕 before superinitializer call.");
            }
            if (isFullyInitializedCheckRequired()) {
                scoper.instanceScope()->initializerUnintializedVariablesCheck(token.position(),
                                                                              "Instance variable \"%s\" must be "
                                                                              "initialized before the use of 🐕.");
            }

            if (!isSelfAllowed()) {
                throw CompilerError(token.position(), "Illegal use of 🐕.");
            }

            auto type = typeContext.calleeType();
            writeBoxingAndTemporary(expectation, type, writer);
            usedSelf = true;
            writer.writeInstruction(INS_GET_THIS);

            return type;
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writer.writeInstruction(INS_GET_NOTHINGNESS);
            return Type::nothingness();
        }
        case E_CLOUD: {
            writeBoxingAndTemporary(expectation, Type::boolean());
            writer.writeInstruction(INS_IS_NOTHINGNESS);
            auto type = parse(stream_.consumeToken(), TypeExpectation(true, false));
            if (!type.optional() && type.type() != TypeContent::Something) {
                throw CompilerError(token.position(), "☁️ can only be used with optionals and ⚪️.");
            }
            scoper.popTemporaryScope();
            return Type::boolean();
        }
        case E_TRAFFIC_LIGHT: {
            writeBoxingAndTemporary(expectation, Type::boolean());
            writer.writeInstruction(INS_IS_ERROR);
            auto type = parse(stream_.consumeToken(), TypeExpectation(true, false));
            if (type.type() != TypeContent::Error) {
                throw CompilerError(token.position(), "🚥 can only be used with 🚨.");
            }
            scoper.popTemporaryScope();
            return Type::boolean();
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writeBoxingAndTemporary(expectation, Type::boolean());
            writer.writeInstruction(INS_SAME_OBJECT);

            parse(stream_.consumeToken(), token, TypeExpectation(Type::someobject()));
            parse(stream_.consumeToken(), token, TypeExpectation(Type::someobject()));

            return Type::boolean();
        }
        case E_WHITE_LARGE_SQUARE: {
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_GET_CLASS_FROM_INSTANCE);
            Type originalType = parse(stream_.consumeToken(), TypeExpectation(false, false, false));
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerError(token.position(), "Can’t get metatype of %s.", string.c_str());
            }
            originalType.setMeta(true);
            writeBoxingAndTemporary(expectation, originalType, insertionPoint);
            return originalType;
        }
        case E_WHITE_SQUARE_BUTTON: {
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX);
            Type originalType = parseTypeDeclarative(typeContext, TypeDynamism::None);
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerError(token.position(), "Can’t get metatype of %s.", string.c_str());
            }
            writer.writeInstruction(originalType.eclass()->index);
            originalType.setMeta(true);
            writeBoxingAndTemporary(expectation, originalType, insertionPoint);
            return originalType;
        }
        case E_BLACK_SQUARE_BUTTON: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder();

            Type originalType = parse(stream_.consumeToken(), TypeExpectation(false, false));
            auto pair = parseTypeAsValue(typeContext, token.position(), TypeExpectation());
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            if (originalType.compatibleTo(type, typeContext)) {
                throw CompilerError(token.position(), "Unnecessary cast.");
            }
            else if (!type.compatibleTo(originalType, typeContext)) {
                auto typeString = type.toString(typeContext, true);
                throw CompilerError(token.position(), "Cast to unrelated type %s will always fail.",
                                    typeString.c_str());
            }

            if (type.type() == TypeContent::Class) {
                auto offset = type.eclass()->numberOfGenericArgumentsWithSuperArguments() - type.eclass()->ownGenericArgumentVariables().size();
                for (size_t i = 0; i < type.eclass()->ownGenericArgumentVariables().size(); i++) {
                    if (!type.eclass()->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments()[i], type) ||
                       !type.genericArguments()[i].compatibleTo(type.eclass()->genericArgumentConstraints()[offset + i], type)) {
                        throw CompilerError(token.position(), "Dynamic casts involving generic type arguments are not "
                                            "possible yet. Please specify the generic argument constraints of the "
                                            "class for compatibility with future versions.");
                    }
                }

                if (originalType.type() == TypeContent::Someobject || originalType.type() == TypeContent::Class) {
                    placeholder.write(INS_DOWNCAST_TO_CLASS);
                    assert(originalType.storageType() == StorageType::Simple && originalType.size() == 1);
                }
                else {
                    assert(originalType.storageType() == StorageType::Box);
                    placeholder.write(INS_CAST_TO_CLASS);
                }
            }
            else if (type.type() == TypeContent::Protocol && isStatic(pair.second)) {
                assert(originalType.storageType() == StorageType::Box);
                placeholder.write(INS_CAST_TO_PROTOCOL);
                writer.writeInstruction(type.protocol()->index);
            }
            else if ((type.type() == TypeContent::ValueType || type.type() == TypeContent::Enum)
                     && isStatic(pair.second)) {
                assert(originalType.storageType() == StorageType::Box);
                placeholder.write(INS_CAST_TO_VALUE_TYPE);
                writer.writeInstruction(type.valueType()->boxIdentifier());
            }
            else {
                auto typeString = pair.first.toString(typeContext, true);
                throw CompilerError(token.position(), "You cannot cast to %s.", typeString.c_str());
            }

            type.setOptional();
            writeBoxingAndTemporary(expectation, type, insertionPoint);
            return type;
        }
        case E_BEER_MUG: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder();

            Type t = parse(stream_.consumeToken(), TypeExpectation(true, false));
            scoper.popTemporaryScope();

            if (!t.optional()) {
                throw CompilerError(token.position(), "🍺 can only be used with optionals.");
            }

            assert(t.isReference());
            t.setReference(false);

            if (t.storageType() == StorageType::Box) {
                placeholder.write(INS_UNWRAP_BOX_OPTIONAL);
                t = t.copyWithoutOptional();
            }
            else {
                placeholder.write(INS_UNWRAP_SIMPLE_OPTIONAL);
                t = t.copyWithoutOptional();
                writer.writeInstruction(t.size());
            }
            writeBoxingAndTemporary(expectation, t, insertionPoint);
            return t;
        }
        case E_METRO: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder();

            Type t = parse(stream_.consumeToken(), TypeExpectation(true, false));
            scoper.popTemporaryScope();

            if (t.type() != TypeContent::Error) {
                throw CompilerError(token.position(), "🚇 can only be used with 🚨.");
            }

            assert(t.isReference());
            t.setReference(false);

            if (t.storageType() == StorageType::Box) {
                placeholder.write(INS_ERROR_CHECK_BOX_OPTIONAL);
                t = t.copyWithoutOptional();
                t.forceBox();
            }
            else {
                placeholder.write(INS_ERROR_CHECK_SIMPLE_OPTIONAL);
                t = t.copyWithoutOptional();
                writer.writeInstruction(t.size());
            }
            Type type = t.genericArguments()[1];
            writeBoxingAndTemporary(expectation, type, insertionPoint);
            return type;
        }

        case E_HOT_PEPPER: {
            writeBoxingAndTemporary(expectation, Type::callableIncomplete());
            Function *function;

            auto placeholder = writer.writeInstructionPlaceholder();
            if (stream_.consumeTokenIf(E_DOUGHNUT)) {
                auto &methodName = stream_.consumeToken();
                auto pair = parseTypeAsValue(typeContext, token.position(), TypeExpectation());
                auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

                if (type.type() == TypeContent::Class) {
                    placeholder.write(INS_CAPTURE_TYPE_METHOD);
                }
                else if (type.type() == TypeContent::ValueType) {
                    notStaticError(pair.second, token.position(), "Value Types");
                    placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
                    writer.writeInstruction(INS_GET_NOTHINGNESS);
                }
                else {
                    throw CompilerError(token.position(), "You can’t capture method calls on this kind of type.");
                }

                function = type.eclass()->getTypeMethod(methodName, type, typeContext);
            }
            else {
                auto &methodName = stream_.consumeToken();
                Type type = parse(stream_.consumeToken(), TypeExpectation(false, false, false));

                if (type.type() == TypeContent::Class) {
                    placeholder.write(INS_CAPTURE_METHOD);
                }
                else if (type.type() == TypeContent::ValueType) {
                    placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
                    if (type.size() > 1) {
                        throw CompilerError(token.position(), "Type not eligible for method capturing.");  // TODO: Improve
                    }
                }
                else {
                    throw CompilerError(token.position(), "You can’t capture method calls on this kind of type.");
                }
                function = type.typeDefinitionFunctional()->getMethod(methodName, type, typeContext);
            }
            checkAccessLevel(function, token.position());
            function->deprecatedWarning(token.position());
            writer.writeInstruction(function->vtiForUse());
            return function->type();
        }
        case E_GRAPES: {
            auto function = new Function(EmojicodeString(E_GRAPES), AccessLevel::Public, true, callable.owningType(),
                                         package_, token.position(), false, EmojicodeString(), false, false, mode);
            parseArgumentList(function, typeContext, true);
            parseReturnType(function, typeContext);
            parseBodyBlock(function);

            function->setVtiProvider(&Function::pureFunctionsProvider);
            package_->registerFunction(function);

            auto closureScoper = CapturingCallableScoper(scoper);
            auto analyzer = writeAndAnalyzeFunction(function, function->writer_, function->owningType(), closureScoper);

            writeBoxingAndTemporary(expectation, Type::callableIncomplete());
            writer.writeInstruction(INS_CLOSURE);
            function->markUsed(false);
            writer.writeInstruction(function->vtiForUse());
            writer.writeInstruction(static_cast<EmojicodeInstruction>(closureScoper.captures().size()));
            writer.writeInstruction(closureScoper.captureSize());
            for (auto capture : closureScoper.captures()) {
                writer.writeInstruction(capture.id);
                writer.writeInstruction(capture.type.size());
                writer.writeInstruction(capture.captureId);
            }
            writer.writeInstruction(analyzer.usedSelfInBody() ? 1 : 0);
            return function->type();
        }
        case E_LOLLIPOP: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_EXECUTE_CALLABLE);

            Type type = parse(stream_.consumeToken(), TypeExpectation(false, false, false));

            if (type.type() != TypeContent::Callable) {
                throw CompilerError(token.position(), "Given value is not callable.");
            }

            for (size_t i = 1; i < type.genericArguments().size(); i++) {
                writer.writeInstruction(type.genericArguments()[i].size());
                parse(stream_.consumeToken(), token, TypeExpectation(type.genericArguments()[i]));
            }

            auto rtype = type.genericArguments()[0];
            writeBoxingAndTemporary(expectation, type, insertionPoint);
            return rtype;
        }
        case E_CHIPMUNK: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            auto &nameToken = stream_.consumeToken(TokenType::Identifier);

            if (mode != FunctionPAGMode::ObjectMethod) {
                throw CompilerError(token.position(), "Not within an object-context.");
            }

            Class *superclass = typeContext.calleeType().eclass()->superclass();

            if (superclass == nullptr) {
                throw CompilerError(token.position(), "Class has no superclass.");
            }

            Function *method = superclass->getMethod(nameToken, Type(superclass, false), typeContext);

            writer.writeInstruction(INS_DISPATCH_SUPER);
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX);
            writer.writeInstruction(superclass->index);
            writer.writeInstruction(method->vtiForUse());

            Type type = parseFunctionCall(typeContext.calleeType(), method, token);
            writeBoxingAndTemporary(expectation, type, insertionPoint);
            return type;
        }
        case E_LARGE_BLUE_DIAMOND: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder();
            auto pair = parseTypeAsValue(typeContext, token.position(), expectation);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            auto &initializerName = stream_.consumeToken(TokenType::Identifier);

            if (type.optional()) {
                throw CompilerError(token.position(), "Optionals cannot be instantiated.");
            }

            Initializer *initializer;
            if (type.type() == TypeContent::Enum) {
                notStaticError(pair.second, token.position(), "Enums");
                placeholder.write(INS_GET_32_INTEGER);

                auto v = type.eenum()->getValueFor(initializerName.value());
                if (!v.first) {
                    throw CompilerError(initializerName.position(), "%s does not have a member named %s.",
                                        type.eenum()->name().utf8().c_str(), initializerName.value().utf8().c_str());
                }
                else if (v.second > UINT32_MAX) {
                    writer.writeInstruction(v.second >> 32);
                    writer.writeInstruction((EmojicodeInstruction)v.second);
                }
                else {
                    writer.writeInstruction((EmojicodeInstruction)v.second);
                }
                type.unbox();
                writeBoxingAndTemporary(expectation, type, insertionPoint);
                return type;
            }
            else if (type.type() == TypeContent::ValueType) {
                notStaticError(pair.second, token.position(), "Value Types");

                placeholder.write(INS_INIT_VT);

                initializer = type.valueType()->getInitializer(initializerName, type, typeContext);
                writer.writeInstruction(initializer->vtiForUse());

                parseFunctionCall(type, initializer, token);
            }
            else if (type.type() == TypeContent::Class) {
                placeholder.write(INS_NEW_OBJECT);

                initializer = type.eclass()->getInitializer(initializerName, type, typeContext);

                if (!isStatic(pair.second) && !initializer->required()) {
                    throw CompilerError(initializerName.position(),
                                        "Only required initializers can be used with dynamic types.");
                }

                initializer->deprecatedWarning(initializerName.position());

                writer.writeInstruction(initializer->vtiForUse());

                parseFunctionCall(type, initializer, token);
            }
            else {
                throw CompilerError(token.position(), "The given type cannot be instantiated.");
            }

            auto constructedType = initializer->constructedType(type);
            writeBoxingAndTemporary(expectation, constructedType, insertionPoint);
            return constructedType;
        }
        case E_DOUGHNUT: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            auto &methodToken = stream_.consumeToken(TokenType::Identifier);

            auto placeholder = writer.writeInstructionPlaceholder();
            auto pair = parseTypeAsValue(typeContext, token.position(), TypeExpectation());
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            if (type.optional()) {
                compilerWarning(token.position(), "You cannot call optionals on 🍬.");
            }

            Function *method;
            if (type.type() == TypeContent::Class) {
                placeholder.write(INS_DISPATCH_TYPE_METHOD);
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                writer.writeInstruction(method->vtiForUse());
            }
            else if ((type.type() == TypeContent::ValueType || type.type() == TypeContent::Enum)
                     && isStatic(pair.second)) {
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                placeholder.write(INS_CALL_FUNCTION);
                writer.writeInstruction(method->vtiForUse());
            }
            else {
                throw CompilerError(token.position(), "You can’t call type methods on %s.",
                                    pair.first.toString(typeContext, true).c_str());
            }
            auto rtype = parseFunctionCall(type, method, token);
            writeBoxingAndTemporary(expectation, rtype, insertionPoint);
            return rtype;
        }
        default: {
            effect = true;
            return parseMethodCall(token, expectation, [this](const TypeExpectation &expectation){
                auto &tobject = stream_.consumeToken();
                return parse(tobject, TypeExpectation(expectation));
            });
        }
    }
    return Type::nothingness();
}

Type FunctionPAG::parseMethodCall(const Token &token, const TypeExpectation &expectation,
                                  std::function<Type(const TypeExpectation &)> callee) {
    auto insertionPoint = writer.getInsertionPoint();
    auto placeholder = writer.writeInstructionPlaceholder();

    auto recompilationPoint = RecompilationPoint(writer, stream_);
    auto recompileWithSimple = [this, recompilationPoint, &callee]() {
        recompilationPoint.restore();
        callee(TypeExpectation(false, false, false));
    };

    auto calleeExpectation = TypeExpectation(true, false);
    Type rtype = callee(calleeExpectation);
    Type type = rtype.resolveOnSuperArgumentsAndConstraints(typeContext);

    if (type.optional()) {
        throw CompilerError(token.position(), "You cannot call methods on optionals.");
    }

    Function *method = nullptr;
    if (type.type() == TypeContent::ValueType) {
        if (type.valueType() == VT_BOOLEAN) {
            switch (token.value()[0]) {
                case E_NEGATIVE_SQUARED_CROSS_MARK:
                    placeholder.write(INS_INVERT_BOOLEAN);
                    recompileWithSimple();
                    return Type::boolean();
                case E_PARTY_POPPER:
                    placeholder.write(INS_OR_BOOLEAN);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::boolean()));
                    return Type::boolean();
                case E_CONFETTI_BALL:
                    placeholder.write(INS_AND_BOOLEAN);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::boolean()));
                    return Type::boolean();
            }
        }
        else if (type.valueType() == VT_INTEGER) {
            switch (token.value()[0]) {
                case E_HEAVY_MINUS_SIGN:
                    placeholder.write(INS_SUBTRACT_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_HEAVY_PLUS_SIGN:
                    placeholder.write(INS_ADD_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_HEAVY_DIVISION_SIGN:
                    placeholder.write(INS_DIVIDE_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_HEAVY_MULTIPLICATION_SIGN:
                    placeholder.write(INS_MULTIPLY_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_LEFT_POINTING_TRIANGLE:
                    placeholder.write(INS_LESS_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::boolean();
                case E_RIGHT_POINTING_TRIANGLE:
                    placeholder.write(INS_GREATER_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::boolean();
                case E_LEFTWARDS_ARROW:
                    placeholder.write(INS_LESS_OR_EQUAL_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::boolean();
                case E_RIGHTWARDS_ARROW:
                    placeholder.write(INS_GREATER_OR_EQUAL_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::boolean();
                case E_PUT_LITTER_IN_ITS_SPACE:
                    placeholder.write(INS_REMAINDER_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_HEAVY_LARGE_CIRCLE:
                    placeholder.write(INS_BINARY_AND_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_ANGER_SYMBOL:
                    placeholder.write(INS_BINARY_OR_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_CROSS_MARK:
                    placeholder.write(INS_BINARY_XOR_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_NO_ENTRY_SIGN:
                    placeholder.write(INS_BINARY_NOT_INTEGER);
                    recompileWithSimple();
                    return Type::integer();
                case E_ROCKET:
                    placeholder.write(INS_INT_TO_DOUBLE);
                    recompileWithSimple();
                    return Type::doubl();
                case E_LEFT_POINTING_BACKHAND_INDEX:
                    placeholder.write(INS_SHIFT_LEFT_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
                case E_RIGHT_POINTING_BACKHAND_INDEX:
                    placeholder.write(INS_SHIFT_RIGHT_INTEGER);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::integer()));
                    return Type::integer();
            }
        }
        else if (type.valueType() == VT_DOUBLE) {
            switch (token.value()[0]) {
                case E_FACE_WITH_STUCK_OUT_TONGUE:
                    placeholder.write(INS_EQUAL_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::boolean();
                case E_HEAVY_MINUS_SIGN:
                    placeholder.write(INS_SUBTRACT_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::doubl();
                case E_HEAVY_PLUS_SIGN:
                    placeholder.write(INS_ADD_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::doubl();
                case E_HEAVY_DIVISION_SIGN:
                    placeholder.write(INS_DIVIDE_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::doubl();
                case E_HEAVY_MULTIPLICATION_SIGN:
                    placeholder.write(INS_MULTIPLY_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::doubl();
                case E_LEFT_POINTING_TRIANGLE:
                    placeholder.write(INS_LESS_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::boolean();
                case E_RIGHT_POINTING_TRIANGLE:
                    placeholder.write(INS_GREATER_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::boolean();
                case E_LEFTWARDS_ARROW:
                    placeholder.write(INS_LESS_OR_EQUAL_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::boolean();
                case E_RIGHTWARDS_ARROW:
                    placeholder.write(INS_GREATER_OR_EQUAL_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::boolean();
                case E_PUT_LITTER_IN_ITS_SPACE:
                    placeholder.write(INS_REMAINDER_DOUBLE);
                    recompileWithSimple();
                    parse(stream_.consumeToken(), token, TypeExpectation(Type::doubl()));
                    return Type::doubl();
            }
        }

        if (token.isIdentifier(E_FACE_WITH_STUCK_OUT_TONGUE) && type.valueType()->isPrimitive()) {
            recompileWithSimple();
            type.setReference(false);
            parse(stream_.consumeToken(), token, TypeExpectation(type));  // TODO: prove correctness
            placeholder.write(INS_EQUAL_PRIMITIVE);

            return Type::boolean();
        }
    }
    else if (type.type() == TypeContent::Enum && token.value()[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
        recompileWithSimple();
        type.setReference(false);
        parse(stream_.consumeToken(), token, TypeExpectation(type));  // Must be of the same type as the callee
        placeholder.write(INS_EQUAL_PRIMITIVE);
        return Type::boolean();
    }

    if (type.type() == TypeContent::ValueType) {
        method = type.valueType()->getMethod(token, type, typeContext);
        mutatingMethodCheck(method, type, calleeExpectation, token.position());
        placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
        writer.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::Protocol) {
        method = type.protocol()->getMethod(token, type, typeContext);
        placeholder.write(INS_DISPATCH_PROTOCOL);
        writer.writeInstruction(type.protocol()->index);
        writer.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::MultiProtocol) {
        for (auto &protocol : type.protocols()) {
            if ((method = protocol.protocol()->lookupMethod(token.value())) != nullptr) {
                placeholder.write(INS_DISPATCH_PROTOCOL);
                writer.writeInstruction(protocol.protocol()->index);
                writer.writeInstruction(method->vtiForUse());
                break;
            }
        }
        if (method == nullptr) {
            throw CompilerError(token.position(), "No type in %s provides a method called %s.",
                                type.toString(typeContext, true).c_str(), token.value().utf8().c_str());
        }
    }
    else if (type.type() == TypeContent::Enum) {
        method = type.eenum()->getMethod(token, type, typeContext);
        placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
        writer.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::Class) {
        method = type.eclass()->getMethod(token, type, typeContext);
        placeholder.write(INS_DISPATCH_METHOD);
        writer.writeInstruction(method->vtiForUse());
    }
    else {
        auto typeString = type.toString(typeContext, true);
        throw CompilerError(token.position(), "You cannot call methods on %s.", typeString.c_str());
    }

    Type rt = parseFunctionCall(type, method, token);
    scoper.popTemporaryScope();

    writeBoxingAndTemporary(expectation, rt, insertionPoint);
    return rt;
}

void FunctionPAG::noReturnError(const SourcePosition &p) {
    if (callable.returnType.type() != TypeContent::Nothingness && !returned) {
        throw CompilerError(p, "An explicit return is missing.");
    }
}

void FunctionPAG::noEffectWarning(const Token &warningToken) {
    if (!effect) {
        compilerWarning(warningToken.position(), "Statement seems to have no effect whatsoever.");
    }
}

void FunctionPAG::notStaticError(TypeAvailability t, const SourcePosition &p, const char *name) {
    if (!isStatic(t)) {
        throw CompilerError(p, "%s cannot be used dynamically.", name);
    }
}

std::pair<Type, TypeAvailability> FunctionPAG::parseTypeAsValue(TypeContext tc, const SourcePosition &p,
                                                                const TypeExpectation &expectation) {
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
        auto type = parse(stream_.consumeToken(), TypeExpectation(false, false, false));
        if (!type.meta()) {
            throw CompilerError(p, "Expected meta type.");
        }
        if (type.optional()) {
            throw CompilerError(p, "🍬 can’t be used as meta type.");
        }
        type.setMeta(false);
        return std::pair<Type, TypeAvailability>(type, TypeAvailability::DynamicAndAvailabale);
    }
    Type ot = parseTypeDeclarative(tc, TypeDynamism::AllKinds, expectation);
    switch (ot.type()) {
        case TypeContent::Reference:
            throw CompilerError(p, "Generic Arguments are not yet available for reflection.");
        case TypeContent::Class:
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX);
            writer.writeInstruction(ot.eclass()->index);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndAvailabale);
        case TypeContent::Self:
            if (mode != FunctionPAGMode::ClassMethod) {
                throw CompilerError(p, "Illegal use of 🐕.");
            }
            writer.writeInstruction(INS_GET_THIS);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::DynamicAndAvailabale);
        case TypeContent::LocalReference:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndUnavailable);
}

FunctionPAG::FunctionPAG(Function &callable, Package *p,
                                                       FunctionPAGMode mode,
                                                       TypeContext typeContext, CallableWriter &writer,
                                                       CallableScoper &scoper)
: AbstractParser(p, callable.tokenStream()),
mode(mode),
callable(callable),
writer(writer),
scoper(scoper),
typeContext(typeContext) {}

void FunctionPAG::analyze() {
    if (hasInstanceScope()) {
        scoper.instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired());
    }
    try {
        Scope &methodScope = scoper.pushScope();
        for (auto variable : callable.arguments) {
            auto &var = methodScope.setLocalVariable(variable.variableName, variable.type, true,
                                                     callable.position());
            var.initialize(writer.writtenInstructions());
        }

        scoper.postArgumentsHook();

        if (isFullyInitializedCheckRequired()) {
            auto initializer = static_cast<Initializer &>(callable);
            for (auto &var : initializer.argumentsToVariables()) {
                if (!scoper.instanceScope()->hasLocalVariable(var)) {
                    throw CompilerError(initializer.position(),
                                        "🍼 was applied to \"%s\" but no matching instance variable was found.",
                                        var.utf8().c_str());
                }
                auto &instanceVariable = scoper.instanceScope()->getLocalVariable(var);
                auto &argumentVariable = methodScope.getLocalVariable(var);
                if (!argumentVariable.type().compatibleTo(instanceVariable.type(), typeContext)) {
                    throw CompilerError(initializer.position(),
                                        "🍼 was applied to \"%s\" but instance variable has incompatible type.",
                                        var.utf8().c_str());
                }
                instanceVariable.initialize(writer.writtenInstructions());
                produceToVariable(ResolvedVariable(instanceVariable, true));
                copyVariableContent(ResolvedVariable(argumentVariable, false));
            }
        }

        while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
            parseStatement(stream_.consumeToken());

            if (returned && !stream_.nextTokenIs(E_WATERMELON)) {
                compilerWarning(stream_.consumeToken().position(), "Dead code.");
                break;
            }
        }

        if (mode == FunctionPAGMode::ObjectInitializer) {
            writer.writeInstruction(INS_RETURN);
            writeBoxingAndTemporary(TypeExpectation(callable.returnType), typeContext.calleeType());
            writer.writeInstruction(INS_GET_THIS);
        }

        scoper.popScopeAndRecommendFrozenVariables(callable.objectVariableInformation(), writer.writtenInstructions());

        if (isFullyInitializedCheckRequired()) {
            auto initializer = static_cast<Initializer &>(callable);
            scoper.instanceScope()->initializerUnintializedVariablesCheck(initializer.position(),
                                                                          "Instance variable \"%s\" must be initialized.");
        }
        else {
            noReturnError(callable.position());
        }
        if (isSuperconstructorRequired()) {
            auto initializer = static_cast<Initializer &>(callable);
            if (!calledSuper && typeContext.calleeType().eclass()->superclass() != nullptr) {
                throw CompilerError(initializer.position(), "Missing call to superinitializer in initializer %s.",
                                    initializer.name().utf8().c_str());
            }
        }
    }
    catch (CompilerError &ce) {
        printError(ce);
    }
}

void FunctionPAG::generateBoxingLayer(BoxingLayer *layer) {
    if (layer->destinationFunction()->returnType.type() != TypeContent::Nothingness) {
        writer.writeInstruction(INS_RETURN);
        writeBoxingAndTemporary(TypeExpectation(layer->returnType), layer->destinationFunction()->returnType);
    }
    switch (layer->owningType().type()) {
        case TypeContent::ValueType:
        case TypeContent::Enum:
            writer.writeInstruction(INS_CALL_CONTEXTED_FUNCTION);
            break;
        case TypeContent::Class:
            writer.writeInstruction(INS_DISPATCH_METHOD);
            break;
        default:
            throw std::logic_error("nonsensial BoxingLayer requested");
    }
    writer.writeInstruction(INS_GET_THIS);
    writer.writeInstruction(layer->destinationFunction()->vtiForUse());
    for (size_t i = 0; i < layer->destinationFunction()->arguments.size(); i++) {
        auto arg = layer->destinationFunction()->arguments[i];
        writer.writeInstruction(layer->arguments[i].type.size());
        writeBoxingAndTemporary(TypeExpectation(arg.type), layer->arguments[i].type);
        auto variable = Variable(layer->arguments[i].type, i, true, EmojicodeString(), layer->position());
        copyVariableContent(ResolvedVariable(variable, false));
    }
    layer->setFullSizeFromArguments();
}

FunctionPAG FunctionPAG::writeAndAnalyzeFunction(Function *function,
                                                                               CallableWriter &writer,
                                                                               Type contextType,
                                                                               CallableScoper &scoper) {
    if (contextType.type() == TypeContent::ValueType) {
        if (function->mutating()) {
            contextType.setMutable(true);
        }
        contextType.setReference();
    }
    else if (contextType.type() == TypeContent::Enum) {
        contextType.setReference();
    }

    auto sca = FunctionPAG(*function, function->package(), function->compilationMode(),
                                          TypeContext(contextType, function), writer, scoper);
    if (function->compilationMode() == FunctionPAGMode::BoxingLayer) {
        sca.generateBoxingLayer(static_cast<BoxingLayer *>(function));
        return sca;
    }
    sca.analyze();
    function->setFullSize(scoper.fullSize());
    return sca;
}
