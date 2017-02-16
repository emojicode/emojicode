//
//  CallableParserAndGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CallableParserAndGenerator.hpp"
#include <cstdlib>
#include <cassert>
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

Type CallableParserAndGenerator::parse(const Token &token, const Token &parentToken, Type type,
                                       Destination des, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = ctargs ? parse(token, Type::nothingness(), des) : parse(token, type, des);
    if (!returnType.compatibleTo(type, typeContext, ctargs)) {
        auto cn = returnType.toString(typeContext, true);
        auto tn = type.toString(typeContext, true);
        throw CompilerError(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
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
    else if (type.type() == TypeContent::Protocol && type.protocol() == PR_ENUMERATEABLE) {
        *elementType = Type(TypeContent::Reference, false, 0, type.protocol()).resolveOn(type);
        return true;
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
        throw CompilerError(token, "Too few generic arguments provided.");
    }

    for (auto var : p->arguments) {
        auto resolved = var.type.resolveOn(typeContext);
        if (inferGenericArguments) {
            writer.writeInstruction(resolved.size(), token);
            auto des = Destination(DestinationMutability::Immutable, StorageType::Box);
            givenArgumentTypes.push_back(parse(stream_.consumeToken(), token, resolved, des, &genericArgsFinders));
        }
        else {
            writer.writeInstruction(resolved.size(), token);
            parse(stream_.consumeToken(), token, resolved,
                  Destination(DestinationMutability::Immutable, resolved.storageType()));
        }
    }

    if (inferGenericArguments) {
        for (size_t i = 0; i < genericArgsFinders.size(); i++) {
            auto commonType = genericArgsFinders[i].getCommonType(token);
            genericArguments.push_back(commonType);
            if (!commonType.compatibleTo(p->genericArgumentConstraints[i], typeContext)) {
                throw CompilerError(token,
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
                throw CompilerError(token, "%s is not compatible to %s.", cn.c_str(), tn.c_str());
            }
        }
    }

    if (p->accessLevel() == AccessLevel::Private) {
        if (this->typeContext.calleeType().type() != p->owningType().type()
            || p->owningType().typeDefinition() != this->typeContext.calleeType().typeDefinition()) {
            throw CompilerError(token, "%s is 🔒.", p->name().utf8().c_str());
        }
    }
    else if (p->accessLevel() == AccessLevel::Protected) {
        if (this->typeContext.calleeType().type() != p->owningType().type()
            || !this->typeContext.calleeType().eclass()->inheritsFrom(p->owningType().eclass())) {
            throw CompilerError(token, "%s is 🔐.", p->name().utf8().c_str());
        }
    }

    return p->returnType.resolveOn(typeContext);
}

void CallableParserAndGenerator::writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                                                    EmojicodeInstruction object,
                                                                    EmojicodeInstruction vt,
                                                                    SourcePosition p) {
    if (!inInstanceScope) {
        writer.writeInstruction(stack, p);
    }
    else {
        writer.writeInstruction(typeContext.calleeType().type() == TypeContent::ValueType ? vt : object, p);
        usedSelf = true;
    }
}

void CallableParserAndGenerator::produceToVariable(ResolvedVariable var, SourcePosition p) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_PRODUCE_WITH_STACK_DESTINATION,
                                       INS_PRODUCE_WITH_OBJECT_DESTINATION, INS_PRODUCE_WITH_VT_DESTINATION, p);
    writer.writeInstruction(var.variable.id(), p);
}

void CallableParserAndGenerator::getVTReference(ResolvedVariable var, SourcePosition p) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_GET_VT_REFERENCE_STACK, INS_GET_VT_REFERENCE_OBJECT,
                                       INS_GET_VT_REFERENCE_VT, p);
    writer.writeInstruction(var.variable.id(), p);
}

void CallableParserAndGenerator::copyVariableContent(ResolvedVariable var, SourcePosition p) {
    if (var.variable.type().size() == 1) {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_SINGLE_STACK, INS_COPY_SINGLE_OBJECT,
                                           INS_COPY_SINGLE_VT, p);
        writer.writeInstruction(var.variable.id(), p);
    }
    else {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_WITH_SIZE_STACK, INS_COPY_WITH_SIZE_OBJECT,
                                           INS_COPY_WITH_SIZE_VT, p);
        writer.writeInstruction(var.variable.id(), p);
        writer.writeInstruction(var.variable.type().size(), p);
    }
}

void CallableParserAndGenerator::flowControlBlock(bool pushScope, std::function<void()> bodyPredicate) {
    scoper.pushInitializationLevel();

    if (pushScope) {
        scoper.pushScope();
    }

    flowControlDepth++;

    auto &token = stream_.requireIdentifier(E_GRAPES);

    auto placeholder = writer.writeInstructionsCountPlaceholderCoin(token);
    if (bodyPredicate) bodyPredicate();
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        parseStatement(stream_.consumeToken());
    }
    stream_.consumeToken();
    placeholder.write();

    effect = true;

    scoper.popScopeAndRecommendFrozenVariables(static_cast<Function &>(callable).objectVariableInformation(),
                                               writer.writtenInstructions());

    scoper.popInitializationLevel();
    flowControlDepth--;
}

void CallableParserAndGenerator::flowControlReturnEnd(FlowControlReturn &fcr) {
    if (returned) {
        fcr.branchReturns++;
        returned = false;
    }
    fcr.branches++;
}

void CallableParserAndGenerator::writeBoxingAndTemporary(Destination des, Type &rtype, SourcePosition p,
                                                         WriteLocation location) const {
    if (rtype.type() == TypeContent::Nothingness) return;
    if (des.simplify(rtype) == StorageType::NoAction) return;

    auto insertionPoint = location.insertionPoint();

    switch (des.simplify(rtype)) {
        case StorageType::SimpleOptional:
            switch (rtype.storageType()) {
                case StorageType::SimpleOptional:
                case StorageType::SimpleIfPossible:
                case StorageType::NoAction:
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
                case StorageType::SimpleIfPossible:
                case StorageType::NoAction:
                    break;
                case StorageType::SimpleOptional:
                    if ((rtype.size() > 3 && !rtype.optional()) || rtype.size() > 4)
                        throw std::logic_error("Beta: Cannot box " + rtype.toString(typeContext, true) + " due to its size.");
                    rtype.forceBox();
                    location.write({ INS_SIMPLE_OPTIONAL_TO_BOX, EmojicodeInstruction(rtype.boxIdentifier()) });
                    break;
                case StorageType::Simple:
                    if ((rtype.size() > 3 && !rtype.optional()) || rtype.size() > 4)
                        throw std::logic_error("Beta: Cannot box " + rtype.toString(typeContext, true) + " due to its size.");
                    rtype.forceBox();
                    location.write({ INS_BOX_PRODUCE, EmojicodeInstruction(rtype.boxIdentifier()) });
                    break;
            }
            break;
        case StorageType::Simple:
            switch (rtype.storageType()) {
                case StorageType::Simple:
                case StorageType::SimpleOptional:
                case StorageType::SimpleIfPossible:
                case StorageType::NoAction:
                    break;
                case StorageType::Box:
                    rtype.unbox();
                    location.write({ INS_UNBOX, EmojicodeInstruction(rtype.size()) });
                    break;
            }
            break;
        case StorageType::NoAction:
        case StorageType::SimpleIfPossible:
            break;
    }

    if (!rtype.isValueReference() && des.isTemporaryReference() && rtype.isValueReferenceWorthy()) {
        scoper.pushTemporaryScope();
        int destinationID = scoper.currentScope().allocateInternalVariable(rtype);
        insertionPoint.insert({ INS_PRODUCE_TO_AND_GET_VT_REFERENCE, EmojicodeInstruction(destinationID) });
        rtype.setValueReference();
    }
    else if (rtype.isValueReference() && !des.isTemporaryReference()) {
        rtype.setValueReference(false);
        insertionPoint.insert({ INS_COPY_REFERENCE, EmojicodeInstruction(rtype.size()) });
    }
}

void CallableParserAndGenerator::parseIfExpression(const Token &token) {
    if (stream_.consumeTokenIf(E_SOFT_ICE_CREAM)) {
        auto placeholder = writer.writeInstructionPlaceholder(token.position());

        auto &varName = stream_.consumeToken(TokenType::Variable);
        if (scoper.currentScope().hasLocalVariable(varName.value())) {
            throw CompilerError(token, "Cannot redeclare variable.");
        }

        auto destination = Destination::temporaryReference();
        Type t = parse(stream_.consumeToken(), destination);
        if (!t.optional()) {
            throw CompilerError(token, "Condition assignment can only be used with optionals.");
        }

        auto box = t.storageType() == StorageType::Box;
        placeholder.write(box ? INS_CONDITIONAL_PRODUCE_BOX : INS_CONDITIONAL_PRODUCE_SIMPLE_OPTIONAL);

        t.setValueReference(false);
        t = t.copyWithoutOptional();

        auto &variable = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
        variable.initialize(writer.writtenInstructions());
        writer.writeInstruction(variable.id(), token.position());
        if (!box) writer.writeInstruction(t.size(), token.position());
    }
    else {
        parse(stream_.consumeToken(TokenType::NoType), token, Type::boolean(), Destination::temporaryReference());
    }
}

std::pair<Type, Destination> CallableParserAndGenerator::parseMethodCallee() {
    auto &tobject = stream_.consumeToken();
    auto destination = Destination::temporaryReference();
    Type type = parse(tobject, Type::nothingness(), destination).resolveOnSuperArgumentsAndConstraints(typeContext);

    return std::make_pair(type, destination);
}

bool CallableParserAndGenerator::isSuperconstructorRequired() const {
    return mode == CallableParserAndGeneratorMode::ObjectInitializer;
}

bool CallableParserAndGenerator::isFullyInitializedCheckRequired() const {
    return mode == CallableParserAndGeneratorMode::ObjectInitializer ||
    mode == CallableParserAndGeneratorMode::ValueTypeInitializer;
}

bool CallableParserAndGenerator::isSelfAllowed() const {
    return mode != CallableParserAndGeneratorMode::Function &&
    mode != CallableParserAndGeneratorMode::ClassMethod;
}

bool CallableParserAndGenerator::hasInstanceScope() const {
    return hasInstanceScope(mode);
}

bool CallableParserAndGenerator::hasInstanceScope(CallableParserAndGeneratorMode mode) {
    return (mode == CallableParserAndGeneratorMode::ObjectMethod ||
            mode == CallableParserAndGeneratorMode::ObjectInitializer ||
            mode == CallableParserAndGeneratorMode::ValueTypeInitializer ||
            mode == CallableParserAndGeneratorMode::ValueTypeMethod);
}

bool CallableParserAndGenerator::isOnlyNothingnessReturnAllowed() const {
    return mode == CallableParserAndGeneratorMode::ObjectInitializer ||
    mode == CallableParserAndGeneratorMode::ValueTypeInitializer;
}

void CallableParserAndGenerator::mutatingMethodCheck(Function *method, Type type, Destination des, SourcePosition p) {
    if (method->mutating()) {
        if (!type.isMutable()) {
            throw CompilerError(p, "%s was marked 🖍 but callee is not mutable.",
                                         method->name().utf8().c_str());
        }
        des.mutateVariable(p);
    }
}

Type CallableParserAndGenerator::parse(const Token &token, Destination &des) {
    return parse(token, Type::nothingness(), des);
}

void CallableParserAndGenerator::parseStatement(const Token &token) {
    if (token.type() == TokenType::Identifier) {
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                if (scoper.currentScope().hasLocalVariable(varName.value())) {
                    throw CompilerError(token, "Cannot redeclare variable.");
                }

                Type t = parseTypeDeclarative(typeContext, TypeDynamism::AllKinds);


                auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, false, varName.position());

                if (t.optional()) {
                    var.initialize(writer.writtenInstructions());
                    writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION, token);
                    writer.writeInstruction(var.id(), token);
                    writer.writeInstruction(INS_GET_NOTHINGNESS, token);
                }
                return;
            }
            case E_CUSTARD: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                Type type = Type::nothingness();
                try {
                    auto rvar = scoper.getVariable(varName.value(), varName.position());

                    rvar.variable.initialize(writer.writtenInstructions());
                    rvar.variable.mutate(varName);

                    produceToVariable(rvar, token);

                    if (rvar.inInstanceScope && !typeContext.calleeType().isMutable()) {
                        throw CompilerError(token.position(),
                                            "Can’t mutate instance variable as method is not marked with 🖍.");
                    }

                    type = rvar.variable.type();
                    parse(stream_.consumeToken(), token, type,
                          Destination(DestinationMutability::Mutable, type.storageType()));
                }
                catch (VariableNotFoundError &vne) {
                    // Not declared, declaring as local variable
                    writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION, token);

                    auto placeholder = writer.writeInstructionPlaceholder(token);

                    auto destination = Destination(DestinationMutability::Mutable, StorageType::SimpleIfPossible);
                    Type t = parse(stream_.consumeToken(), Type::nothingness(), destination);
                    auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, false, varName.position());
                    var.initialize(writer.writtenInstructions());
                    placeholder.write(var.id());
                }
                return;
            }
            case E_SOFT_ICE_CREAM: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                if (scoper.currentScope().hasLocalVariable(varName.value())) {
                    throw CompilerError(token, "Cannot redeclare variable.");
                }

                writer.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION, token);

                auto placeholder = writer.writeInstructionPlaceholder(token);

                auto destination = Destination(DestinationMutability::Immutable, StorageType::SimpleIfPossible);
                Type t = parse(stream_.consumeToken(), Type::nothingness(), destination);
                auto &var = scoper.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
                var.initialize(writer.writtenInstructions());
                placeholder.write(var.id());
                return;
            }
            case E_COOKING:
            case E_CHOCOLATE_BAR: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                auto var = scoper.getVariable(varName.value(), varName.position());

                var.variable.uninitalizedError(varName);
                var.variable.mutate(varName);

                if (!var.variable.type().compatibleTo(Type::integer(), typeContext)) {
                    throw CompilerError(token, "%s can only operate on 🚂 variables.", token.value().utf8().c_str());
                }

                produceToVariable(var, token);

                if (token.isIdentifier(E_COOKING)) {
                    writer.writeInstruction(INS_DECREMENT, token);
                }
                else {
                    writer.writeInstruction(INS_INCREMENT, token);
                }
                return;
            }
            case E_TANGERINE: {
                writer.writeInstruction(INS_IF, token);

                auto placeholder = writer.writeInstructionsCountPlaceholderCoin(token);
                auto fcr = FlowControlReturn();

                scoper.pushScope();
                parseIfExpression(token);
                flowControlBlock(false);
                flowControlReturnEnd(fcr);

                while (stream_.consumeTokenIf(E_LEMON)) {
                    writer.writeInstruction(0x1, token);

                    scoper.pushScope();
                    parseIfExpression(token);
                    flowControlBlock(false);
                    flowControlReturnEnd(fcr);
                }

                if (stream_.consumeTokenIf(E_STRAWBERRY)) {
                    writer.writeInstruction(0x2, token);
                    flowControlBlock();
                    flowControlReturnEnd(fcr);
                }
                else {
                    writer.writeInstruction(0x0, token);
                    fcr.branches++;  // The else branch always exists. Theoretically at least.
                }

                placeholder.write();

                returned = fcr.returned();
                
                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
                writer.writeInstruction(INS_REPEAT_WHILE, token);

                parseIfExpression(token);
                flowControlBlock();
                returned = false;

                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
                scoper.pushScope();

                auto &variableToken = stream_.consumeToken(TokenType::Variable);

                auto insertionPoint = writer.getInsertionPoint();
                auto destination = Destination(DestinationMutability::Unknown, StorageType::NoAction);
                Type iteratee = parse(stream_.consumeToken(), Type::nothingness(), destination);

                Type itemType = Type::nothingness();

                if (iteratee.type() == TypeContent::Class && iteratee.eclass() == CL_LIST) {
                    // If the iteratee is a list, the Real-Time Engine has some special sugar
                    int internalId = scoper.currentScope().allocateInternalVariable(iteratee);
                    writer.writeInstruction(internalId, token);  // Internally needed
                    auto type = Type(TypeContent::Reference, false, 0, CL_LIST).resolveOn(iteratee);
                    auto &var = scoper.currentScope().setLocalVariable(variableToken.value(), type,
                                                                       true, variableToken.position());
                    var.initialize(writer.writtenInstructions());
                    auto destination = Destination::temporaryReference();
                    writeBoxingAndTemporary(destination, iteratee, token.position(), insertionPoint);
                    insertionPoint.insert({ 0x65, static_cast<unsigned int>(var.id()) });
                    flowControlBlock(false);
                }
                else if (iteratee.type() == TypeContent::ValueType &&
                         iteratee.valueType()->name()[0] == E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE) {
                    // If the iteratee is a range, the Real-Time Engine also has some special sugar
                    auto &var = scoper.currentScope().setLocalVariable(variableToken.value(), Type::integer(), true,
                                                                       variableToken.position());
                    var.initialize(writer.writtenInstructions());
                    auto destination = Destination::temporaryReference();
                    writeBoxingAndTemporary(destination, iteratee, token.position(), insertionPoint);
                    insertionPoint.insert({ 0x66, static_cast<unsigned int>(var.id()) });
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

                    auto destination = Destination::temporaryReference(StorageType::Box);
                    writeBoxingAndTemporary(destination, iteratee, token.position(), insertionPoint);
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
                    throw CompilerError(token, "%s does not conform to s🔂.", iterateeString.c_str());
                }

                returned = false;
                return;
            }
            case E_GOAT: {
                if (!isSuperconstructorRequired()) {
                    throw CompilerError(token, "🐐 can only be used inside initializers.");
                }
                if (!typeContext.calleeType().eclass()->superclass()) {
                    throw CompilerError(token, "🐐 can only be used if the eclass inherits from another.");
                }
                if (calledSuper) {
                    throw CompilerError(token, "You may not call more than one superinitializer.");
                }
                if (flowControlDepth) {
                    throw CompilerError(token, "You may not put a call to a superinitializer in a flow control structure.");
                }

                scoper.instanceScope()->initializerUnintializedVariablesCheck(token, "Instance variable \"%s\" must be "
                                                                              "initialized before superinitializer.");

                writer.writeInstruction(INS_SUPER_INITIALIZER, token);

                Class *eclass = typeContext.calleeType().eclass();

                writer.writeInstruction(INS_GET_CLASS_FROM_INDEX, token);
                writer.writeInstruction(eclass->superclass()->index, token);

                auto &initializerToken = stream_.consumeToken(TokenType::Identifier);

                auto initializer = eclass->superclass()->getInitializer(initializerToken, Type(eclass, false), typeContext);

                writer.writeInstruction(initializer->vtiForUse(), token);

                parseFunctionCall(typeContext.calleeType(), initializer, token);
                
                calledSuper = true;
                
                return;
            }
            case E_RED_APPLE: {
                writer.writeInstruction(INS_RETURN, token);

                if (isOnlyNothingnessReturnAllowed()) {
                    if (static_cast<Initializer &>(callable).canReturnNothingness) {
                        parse(stream_.consumeToken(), token, Type::nothingness(),
                              Destination(DestinationMutability::Unknown, StorageType::Simple));
                        return;
                    }
                    else {
                        throw CompilerError(token, "🍎 cannot be used inside an initializer.");
                    }
                }

                parse(stream_.consumeToken(), token, callable.returnType,
                      Destination(DestinationMutability::Immutable, callable.returnType.storageType()));
                returned = true;
                return;
            }
        }
    }

    // Must be an expression
    effect = false;
    auto des = Destination::temporaryReference();
    auto type = parse(token, Type::nothingness(), des);
    noEffectWarning(token);
    scoper.clearAllTemporaryScopes();
}

Type CallableParserAndGenerator::parse(const Token &token, Type expectation, Destination &des) {
    switch (token.type()) {
        case TokenType::String: {
            writeBoxingAndTemporary(des, Type(CL_STRING, false), token.position());
            writer.writeInstruction(INS_GET_STRING_POOL, token);
            writer.writeInstruction(StringPool::theStringPool().poolString(token.value()), token);
            return Type(CL_STRING, false);
        }
        case TokenType::BooleanTrue:
            writeBoxingAndTemporary(des, Type::boolean(), token.position());
            writer.writeInstruction(INS_GET_TRUE, token);
            return Type::boolean();
        case TokenType::BooleanFalse:
            writeBoxingAndTemporary(des, Type::boolean(), token.position());
            writer.writeInstruction(INS_GET_FALSE, token);
            return Type::boolean();
        case TokenType::Integer: {
            long long value = std::stoll(token.value().utf8(), 0, 0);

            if (expectation.type() == TypeContent::ValueType && expectation.valueType() == VT_DOUBLE) {
                writeBoxingAndTemporary(des, Type::doubl(), token.position());
                writer.writeInstruction(INS_GET_DOUBLE, token);
                writer.writeDoubleCoin(value, token);
                return Type::doubl();
            }

            if (std::llabs(value) > INT32_MAX) {
                writeBoxingAndTemporary(des, Type::integer(), token.position());
                writer.writeInstruction(INS_GET_64_INTEGER, token);

                writer.writeInstruction(value >> 32, token);
                writer.writeInstruction(static_cast<EmojicodeInstruction>(value), token);

                return Type::integer();
            }
            else {
                writeBoxingAndTemporary(des, Type::integer(), token.position());
                writer.writeInstruction(INS_GET_32_INTEGER, token);
                value += INT32_MAX;
                writer.writeInstruction(value, token);

                return Type::integer();
            }
        }
        case TokenType::Double: {
            writeBoxingAndTemporary(des, Type::doubl(), token.position());
            writer.writeInstruction(INS_GET_DOUBLE, token);

            double d = std::stod(token.value().utf8());
            writer.writeDoubleCoin(d, token);

            return Type::doubl();
        }
        case TokenType::Symbol:
            writeBoxingAndTemporary(des, Type::symbol(), token.position());
            writer.writeInstruction(INS_GET_SYMBOL, token);
            writer.writeInstruction(token.value()[0], token);
            return Type::symbol();
        case TokenType::Variable: {
            auto rvar = scoper.getVariable(token.value(), token.position());
            rvar.variable.uninitalizedError(token);

            auto storageType = des.simplify(rvar.variable.type());
            auto returnType = rvar.variable.type();

            if (storageType == rvar.variable.type().storageType() &&
                des.isTemporaryReference() && rvar.variable.type().isValueReferenceWorthy()) {
                getVTReference(rvar, token.position());
                returnType.setValueReference();
            }
            else {
                writeBoxingAndTemporary(des, returnType, token.position(), writer);
                copyVariableContent(rvar, token.position());
                if (des.isMutable()) {
                    returnType.setMutable(true);
                }
            }

            des.setMutatedVariable(rvar.variable);
            return returnType;
        }
        case TokenType::Identifier:
            return parseIdentifier(token, expectation, des);
        case TokenType::DocumentationComment:
            throw CompilerError(token, "Misplaced documentation comment.");
        case TokenType::NoType:
        case TokenType::Comment:
            break;
    }
    throw std::logic_error("Cannot determine expression’s return type.");
}

Type CallableParserAndGenerator::parseIdentifier(const Token &token, Type expectation, Destination &des) {
    switch (token.value()[0]) {
        case E_LEMON:
        case E_STRAWBERRY:
        case E_WATERMELON:
        case E_AUBERGINE:
        case E_SHORTCAKE:
        case E_CUSTARD:
        case E_SOFT_ICE_CREAM:
        case E_COOKING:
        case E_CHOCOLATE_BAR:
        case E_TANGERINE:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS:
        case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY:
        case E_GOAT:
        case E_RED_APPLE:
            throw CompilerError(token, "Unexpected statement %s.", token.value().utf8().c_str());
        case E_COOKIE: {
            writeBoxingAndTemporary(des, Type(CL_STRING, false), token.position());
            writer.writeInstruction(0x52, token);
            auto placeholder = writer.writeInstructionPlaceholder(token);

            int stringCount = 0;

            while (stream_.nextTokenIsEverythingBut(E_COOKIE)) {
                auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
                parse(stream_.consumeToken(), token, Type(CL_STRING, false), destination);
                stringCount++;
            }
            stream_.consumeToken(TokenType::Identifier);

            if (stringCount == 0) {
                throw CompilerError(token, "An empty 🍪 is invalid.");
            }

            placeholder.write(stringCount);
            return Type(CL_STRING, false);
        }
        case E_ICE_CREAM: {
            writeBoxingAndTemporary(des, Type(CL_LIST, false), token.position());
            writer.writeInstruction(0x51, token);

            auto placeholder = writer.writeInstructionsCountPlaceholderCoin(token);

            Type type = Type(CL_LIST, false);
            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_LIST) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    auto destination = Destination(DestinationMutability::Unknown, StorageType::Box);
                    parse(stream_.consumeToken(), token, listType, destination);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    auto destination = Destination(DestinationMutability::Unknown, StorageType::Box);
                    ct.addType(parse(stream_.consumeToken(), destination), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = ct.getCommonType(token);
            }

            placeholder.write();
            return type;
        }
        case E_HONEY_POT: {
            writeBoxingAndTemporary(des, Type(CL_LIST, false), token.position());
            writer.writeInstruction(0x50, token);

            auto placeholder = writer.writeInstructionsCountPlaceholderCoin(token);
            Type type = Type(CL_DICTIONARY, false);

            if (expectation.type() == TypeContent::Class && expectation.eclass() == CL_DICTIONARY) {
                auto listType = expectation.genericArguments[0];
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
                    parse(stream_.consumeToken(), token, Type(CL_STRING, false), destination);
                    destination = Destination(DestinationMutability::Unknown, StorageType::Box);
                    parse(stream_.consumeToken(), token, listType, destination);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = listType;
            }
            else {
                CommonTypeFinder ct;
                while (stream_.nextTokenIsEverythingBut(E_AUBERGINE)) {
                    auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
                    parse(stream_.consumeToken(), token, Type(CL_STRING, false), destination);
                    destination = Destination(DestinationMutability::Unknown, StorageType::Box);
                    ct.addType(parse(stream_.consumeToken(), destination), typeContext);
                }
                stream_.consumeToken(TokenType::Identifier);
                type.genericArguments[0] = ct.getCommonType(token);
            }

            placeholder.write();
            return type;
        }
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR: {
            Type type = Type::nothingness();
            package_->fetchRawType(EmojicodeString(E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE), globalNamespace,
                                   false, token, &type);

            writeBoxingAndTemporary(des, type, token.position(), writer);

            writer.writeInstruction(INS_INIT_VT, token);
            auto initializer = type.valueType()->getInitializer(token, type, typeContext);
            writer.writeInstruction(initializer->vtiForUse(), token);

            parseFunctionCall(type, initializer, token);

            return type;
        }
        case E_DOG: {
            if (isSuperconstructorRequired() && !calledSuper &&
                static_cast<Initializer &>(callable).owningType().eclass()->superclass()) {
                throw CompilerError(token, "Attempt to use 🐕 before superinitializer call.");
            }
            if (isFullyInitializedCheckRequired()) {
                scoper.instanceScope()->initializerUnintializedVariablesCheck(token, "Instance variable \"%s\" must be "
                                                                              "initialized before the use of 🐕.");
            }

            if (!isSelfAllowed()) {
                throw CompilerError(token, "Illegal use of 🐕.");
            }

            auto type = typeContext.calleeType();
            writeBoxingAndTemporary(des, type, token.position(), writer);
            usedSelf = true;
            writer.writeInstruction(INS_GET_THIS, token);

            return type;
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writer.writeInstruction(INS_GET_NOTHINGNESS, token);
            return Type::nothingness();
        }
        case E_CLOUD: {
            writeBoxingAndTemporary(des, Type::boolean(), token.position());
            writer.writeInstruction(INS_IS_NOTHINGNESS, token);
            auto destination = Destination::temporaryReference();
            auto type = parse(stream_.consumeToken(), destination);
            if (!type.optional() && type.type() != TypeContent::Something) {
                throw CompilerError(token, "☁️ can only be used on optionals and ⚪️.");
            }
            scoper.popTemporaryScope();
            return Type::boolean();
        }
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE: {
            writeBoxingAndTemporary(des, Type::boolean(), token.position());
            writer.writeInstruction(INS_SAME_OBJECT, token);

            auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
            parse(stream_.consumeToken(), token, Type::someobject(), destination);
            parse(stream_.consumeToken(), token, Type::someobject(), destination);

            return Type::boolean();
        }
        case E_WHITE_LARGE_SQUARE: {
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_GET_CLASS_FROM_INSTANCE, token);
            auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
            Type originalType = parse(stream_.consumeToken(), destination);
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerError(token, "Can’t get metatype of %s.", string.c_str());
            }
            originalType.setMeta(true);
            writeBoxingAndTemporary(des, originalType, token.position(), insertionPoint);
            return originalType;
        }
        case E_WHITE_SQUARE_BUTTON: {
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX, token);
            Type originalType = parseTypeDeclarative(typeContext, TypeDynamism::None);
            if (!originalType.allowsMetaType()) {
                auto string = originalType.toString(typeContext, true);
                throw CompilerError(token, "Can’t get metatype of %s.", string.c_str());
            }
            writer.writeInstruction(originalType.eclass()->index, token);
            originalType.setMeta(true);
            writeBoxingAndTemporary(des, originalType, token.position(), insertionPoint);
            return originalType;
        }
        case E_BLACK_SQUARE_BUTTON: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder(token);

            auto destination = Destination(DestinationMutability::Unknown, StorageType::SimpleIfPossible);
            Type originalType = parse(stream_.consumeToken(), destination);
            auto pair = parseTypeAsValue(typeContext, token);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            if (originalType.compatibleTo(type, typeContext)) {
                throw CompilerError(token, "Unnecessary cast.");
            }
            else if (!type.compatibleTo(originalType, typeContext)) {
                auto typeString = type.toString(typeContext, true);
                throw CompilerError(token, "Cast to unrelated type %s will always fail.", typeString.c_str());
            }

            if (type.type() == TypeContent::Class) {
                auto offset = type.eclass()->numberOfGenericArgumentsWithSuperArguments() - type.eclass()->numberOfOwnGenericArguments();
                for (size_t i = 0; i < type.eclass()->numberOfOwnGenericArguments(); i++) {
                    if (!type.eclass()->genericArgumentConstraints()[offset + i].compatibleTo(type.genericArguments[i], type) ||
                       !type.genericArguments[i].compatibleTo(type.eclass()->genericArgumentConstraints()[offset + i], type)) {
                        throw CompilerError(token, "Dynamic casts involving generic type arguments are not possible "
                                            "yet. Please specify the generic argument constraints of the class for "
                                            "compatibility with future versions.");
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
                writer.writeInstruction(type.protocol()->index, token);
            }
            else if (type.type() == TypeContent::ValueType && isStatic(pair.second)) {
                assert(originalType.storageType() == StorageType::Box);
                placeholder.write(INS_CAST_TO_VALUE_TYPE);
                writer.writeInstruction(type.valueType()->boxIdentifier(), token);
            }
            else {
                auto typeString = pair.first.toString(typeContext, true);
                throw CompilerError(token, "You cannot cast to %s.", typeString.c_str());
            }

            type.setOptional();
            writeBoxingAndTemporary(des, type, token.position(), insertionPoint);
            return type;
        }
        case E_BEER_MUG: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder(token);

            auto destination = Destination::temporaryReference();
            Type t = parse(stream_.consumeToken(), Type::nothingness(), destination);
            scoper.popTemporaryScope();

            if (!t.optional()) {
                throw CompilerError(token, "🍺 can only be used with optionals.");
            }

            assert(t.isValueReference());
            t.setValueReference(false);

            if (t.storageType() == StorageType::Box) {
                placeholder.write(INS_UNWRAP_BOX_OPTIONAL);
                t = t.copyWithoutOptional();
            }
            else {
                placeholder.write(INS_UNWRAP_SIMPLE_OPTIONAL);
                t = t.copyWithoutOptional();
                writer.writeInstruction(t.size(), token.position());
            }
            writeBoxingAndTemporary(des, t, token.position(), insertionPoint);
            return t;
        }
        case E_CLINKING_BEER_MUGS: {
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_OPTIONAL_DISPATCH_METHOD, token);

            auto &methodToken = stream_.consumeToken();

            auto calleePair = parseMethodCallee();
            Type type = calleePair.first;

            if (!type.optional()) {
                throw CompilerError(token, "🍻 may only be used on 🍬.");
            }

            auto method = type.eclass()->getMethod(methodToken, type, typeContext);

            if (type.type() == TypeContent::ValueType) {
                mutatingMethodCheck(method, type, calleePair.second, token);
            }

            writer.writeInstruction(method->vtiForUse(), token);

            auto placeholder = writer.writeInstructionsCountPlaceholderCoin(token);
            parseFunctionCall(type, method, token);

            placeholder.write();

            Type returnType = method->returnType.resolveOn(typeContext);
            returnType.setOptional();
            scoper.popTemporaryScope();

            writeBoxingAndTemporary(des, returnType, token.position(), insertionPoint);
            return returnType;
        }
        case E_HOT_PEPPER: {
            writeBoxingAndTemporary(des, Type::callableIncomplete(), token.position());
            Function *function;

            auto placeholder = writer.writeInstructionPlaceholder(token);
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
                    writer.writeInstruction(INS_GET_NOTHINGNESS, token);
                }
                else {
                    throw CompilerError(token, "You can’t capture method calls on this kind of type.");
                }

                function = type.eclass()->getTypeMethod(methodName, type, typeContext);
            }
            else {
                auto &methodName = stream_.consumeToken();
                auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
                Type type = parse(stream_.consumeToken(), destination);

                if (type.type() == TypeContent::Class) {
                    placeholder.write(INS_CAPTURE_METHOD);
                }
                else if (type.type() == TypeContent::ValueType) {
                    placeholder.write(INS_CAPTURE_CONTEXTED_FUNCTION);
                }
                else {
                    throw CompilerError(token, "You can’t capture method calls on this kind of type.");
                }
                function = type.typeDefinitionFunctional()->getMethod(methodName, type, typeContext);
            }

            function->deprecatedWarning(token);
            writer.writeInstruction(function->vtiForUse(), token);
            return function->type();
        }
        case E_GRAPES: {
            writeBoxingAndTemporary(des, Type::callableIncomplete(), token.position());
            writer.writeInstruction(INS_CLOSURE, token);

            auto function = Closure(token.position());
            parseArgumentList(&function, typeContext);
            parseReturnType(&function, typeContext);
            parseBody(&function);

            auto variableCountPlaceholder = writer.writeInstructionPlaceholder(token);
            auto coinCountPlaceholder = writer.writeInstructionsCountPlaceholderCoin(token);

            auto closureScoper = CapturingCallableScoper(scoper);

            // TODO: Intializer
            auto analyzer = CallableParserAndGenerator(function, package_, mode, typeContext, writer, closureScoper);
            analyzer.analyze();

            coinCountPlaceholder.write();
            variableCountPlaceholder.write(closureScoper.fullSize());
            writer.writeInstruction(static_cast<EmojicodeInstruction>(function.arguments.size())
                                    | (analyzer.usedSelfInBody() ? 1 << 16 : 0), token);

            writer.writeInstruction(static_cast<EmojicodeInstruction>(closureScoper.captures().size()), token);
            writer.writeInstruction(closureScoper.captureSize(), token);
            for (auto capture : closureScoper.captures()) {
                writer.writeInstruction(capture.id, token);
                writer.writeInstruction(capture.type.size(), token);
                writer.writeInstruction(capture.captureId, token);
            }

            return function.type();
        }
        case E_LOLLIPOP: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            writer.writeInstruction(INS_EXECUTE_CALLABLE, token);

            auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
            Type type = parse(stream_.consumeToken(), destination);

            if (type.type() != TypeContent::Callable) {
                throw CompilerError(token, "Given value is not callable.");
            }

            for (int i = 1; i < type.genericArguments.size(); i++) {
                writer.writeInstruction(type.genericArguments[i].size(), token);
                parse(stream_.consumeToken(), token, type.genericArguments[i],
                      Destination(DestinationMutability::Immutable, type.genericArguments[i].storageType()));
            }

            auto rtype = type.genericArguments[0];
            writeBoxingAndTemporary(des, type, token.position(), insertionPoint);
            return rtype;
        }
        case E_CHIPMUNK: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            auto &nameToken = stream_.consumeToken();

            if (mode != CallableParserAndGeneratorMode::ObjectMethod) {
                throw CompilerError(token, "Not within an object-context.");
            }

            Class *superclass = typeContext.calleeType().eclass()->superclass();

            if (superclass == nullptr) {
                throw CompilerError(token, "Class has no superclass.");
            }

            Function *method = superclass->getMethod(nameToken, Type(superclass, false), typeContext);

            writer.writeInstruction(INS_DISPATCH_SUPER, token);
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX, token);
            writer.writeInstruction(superclass->index, token);
            writer.writeInstruction(method->vtiForUse(), token);

            Type type = parseFunctionCall(typeContext.calleeType(), method, token);
            writeBoxingAndTemporary(des, type, token.position(), insertionPoint);
            return type;
        }
        case E_LARGE_BLUE_DIAMOND: {
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder(token);
            auto pair = parseTypeAsValue(typeContext, token, expectation);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            auto &initializerName = stream_.consumeToken(TokenType::Identifier);

            if (type.optional()) {
                throw CompilerError(token, "Optionals cannot be instantiated.");
            }

            if (type.type() == TypeContent::Enum) {
                notStaticError(pair.second, token, "Enums");
                placeholder.write(INS_GET_32_INTEGER);

                auto v = type.eenum()->getValueFor(initializerName.value());
                if (!v.first) {
                    throw CompilerError(initializerName, "%s does not have a member named %s.",
                                                 type.eenum()->name().utf8().c_str(),
                                                 initializerName.value().utf8().c_str());
                }
                else if (v.second > UINT32_MAX) {
                    writer.writeInstruction(v.second >> 32, token);
                    writer.writeInstruction((EmojicodeInstruction)v.second, token);
                }
                else {
                    writer.writeInstruction((EmojicodeInstruction)v.second, token);
                }
            }
            else if (type.type() == TypeContent::ValueType) {
                notStaticError(pair.second, token, "Value Types");

                placeholder.write(INS_INIT_VT);

                auto initializer = type.valueType()->getInitializer(initializerName, type, typeContext);
                writer.writeInstruction(initializer->vtiForUse(), token);

                parseFunctionCall(type, initializer, token);
                if (initializer->canReturnNothingness) {
                    type.setOptional();
                }
            }
            else if (type.type() == TypeContent::Class) {
                placeholder.write(INS_NEW_OBJECT);

                Initializer *initializer = type.eclass()->getInitializer(initializerName, type, typeContext);

                if (!isStatic(pair.second) && !initializer->required) {
                    throw CompilerError(initializerName,
                                                 "Only required initializers can be used with dynamic types.");
                }

                initializer->deprecatedWarning(initializerName);

                writer.writeInstruction(initializer->vtiForUse(), token);

                parseFunctionCall(type, initializer, token);

                if (initializer->canReturnNothingness) {
                    type.setOptional();
                }
            }
            else {
                throw CompilerError(token, "The given type cannot be instantiated.");
            }

            type.unbox();

            writeBoxingAndTemporary(des, type, token.position(), insertionPoint);
            return type;
        }
        case E_DOUGHNUT: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            auto &methodToken = stream_.consumeToken(TokenType::Identifier);

            auto placeholder = writer.writeInstructionPlaceholder(token);
            auto pair = parseTypeAsValue(typeContext, token);
            auto type = pair.first.resolveOnSuperArgumentsAndConstraints(typeContext);

            if (type.optional()) {
                compilerWarning(token, "You cannot call optionals on 🍬.");
            }

            Function *method;
            if (type.type() == TypeContent::Class) {
                placeholder.write(INS_DISPATCH_TYPE_METHOD);
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else if ((type.type() == TypeContent::ValueType || type.type() == TypeContent::Enum)
                     && isStatic(pair.second)) {
                method = type.typeDefinitionFunctional()->getTypeMethod(methodToken, type, typeContext);
                placeholder.write(INS_CALL_FUNCTION);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else {
                throw CompilerError(token, "You can’t call type methods on %s.",
                                    pair.first.toString(typeContext, true).c_str());
            }
            auto rtype = parseFunctionCall(type, method, token);
            writeBoxingAndTemporary(des, rtype, token.position(), insertionPoint);
            return rtype;
        }
        default: {
            effect = true;
            auto insertionPoint = writer.getInsertionPoint();
            auto placeholder = writer.writeInstructionPlaceholder(token);

            auto calleePair = parseMethodCallee();
            Type type = calleePair.first;

            if (type.optional()) {
                throw CompilerError(token, "You cannot call methods on optionals.");
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
                            parse(stream_.consumeToken(), token, Type::boolean(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_CONFETTI_BALL:
                            placeholder.write(INS_AND_BOOLEAN);
                            parse(stream_.consumeToken(), token, Type::boolean(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                    }
                }
                else if (type.valueType() == VT_INTEGER) {
                    switch (token.value()[0]) {
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(INS_SUBTRACT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(INS_ADD_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(INS_DIVIDE_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(INS_MULTIPLY_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(INS_LESS_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(INS_GREATER_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(INS_LESS_OR_EQUAL_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(INS_GREATER_OR_EQUAL_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(INS_REMAINDER_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_HEAVY_LARGE_CIRCLE:
                            placeholder.write(INS_BINARY_AND_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_ANGER_SYMBOL:
                            placeholder.write(INS_BINARY_OR_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_CROSS_MARK:
                            placeholder.write(INS_BINARY_XOR_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_NO_ENTRY_SIGN:
                            placeholder.write(INS_BINARY_NOT_INTEGER);
                            return Type::integer();
                        case E_ROCKET:
                            placeholder.write(INS_INT_TO_DOUBLE);
                            return Type::doubl();
                        case E_LEFT_POINTING_BACKHAND_INDEX:
                            placeholder.write(INS_SHIFT_LEFT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                        case E_RIGHT_POINTING_BACKHAND_INDEX:
                            placeholder.write(INS_SHIFT_RIGHT_INTEGER);
                            parse(stream_.consumeToken(), token, Type::integer(),
                                  Destination::temporaryReference());
                            return Type::integer();
                    }
                }
                else if (type.valueType() == VT_DOUBLE) {
                    switch (token.value()[0]) {
                        case E_FACE_WITH_STUCK_OUT_TONGUE:
                            placeholder.write(INS_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_HEAVY_MINUS_SIGN:
                            placeholder.write(INS_SUBTRACT_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::doubl();
                        case E_HEAVY_PLUS_SIGN:
                            placeholder.write(INS_ADD_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::doubl();
                        case E_HEAVY_DIVISION_SIGN:
                            placeholder.write(INS_DIVIDE_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::doubl();
                        case E_HEAVY_MULTIPLICATION_SIGN:
                            placeholder.write(INS_MULTIPLY_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::doubl();
                        case E_LEFT_POINTING_TRIANGLE:
                            placeholder.write(INS_LESS_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_RIGHT_POINTING_TRIANGLE:
                            placeholder.write(INS_GREATER_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_LEFTWARDS_ARROW:
                            placeholder.write(INS_LESS_OR_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_RIGHTWARDS_ARROW:
                            placeholder.write(INS_GREATER_OR_EQUAL_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::boolean();
                        case E_PUT_LITTER_IN_ITS_SPACE:
                            placeholder.write(INS_REMAINDER_DOUBLE);
                            parse(stream_.consumeToken(), token, Type::doubl(),
                                  Destination::temporaryReference());
                            return Type::doubl();
                    }
                }

                if (token.isIdentifier(E_FACE_WITH_STUCK_OUT_TONGUE)) {
                    parse(stream_.consumeToken(), token, type,
                          Destination(DestinationMutability::Unknown, StorageType::Simple));
                    placeholder.write(INS_EQUAL_PRIMITIVE);
                    return Type::boolean();
                }

                method = type.valueType()->getMethod(token, type, typeContext);
                mutatingMethodCheck(method, type, calleePair.second, token);
                placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Protocol) {
                method = type.protocol()->getMethod(token, type, typeContext);
                placeholder.write(INS_DISPATCH_PROTOCOL);
                writer.writeInstruction(type.protocol()->index, token);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Enum) {
                if (token.value()[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
                    auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
                    parse(stream_.consumeToken(), token, type, destination);  // Must be of the same type as the callee
                    placeholder.write(INS_EQUAL_PRIMITIVE);
                    return Type::boolean();
                }
                method = type.eenum()->getMethod(token, type, typeContext);
                placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else if (type.type() == TypeContent::Class) {
                method = type.eclass()->getMethod(token, type, typeContext);
                placeholder.write(INS_DISPATCH_METHOD);
                writer.writeInstruction(method->vtiForUse(), token);
            }
            else {
                auto typeString = type.toString(typeContext, true);
                throw CompilerError(token, "You cannot call methods on %s.", typeString.c_str());
            }

            Type rt = parseFunctionCall(type, method, token);
            scoper.popTemporaryScope();

            writeBoxingAndTemporary(des, rt, token.position(), insertionPoint);
            return rt;
        }
    }
    return Type::nothingness();
}

void CallableParserAndGenerator::noReturnError(SourcePosition p) {
    if (callable.returnType.type() != TypeContent::Nothingness && !returned) {
        throw CompilerError(p, "An explicit return is missing.");
    }
}

void CallableParserAndGenerator::noEffectWarning(const Token &warningToken) {
    if (!effect) {
        compilerWarning(warningToken, "Statement seems to have no effect whatsoever.");
    }
}

void CallableParserAndGenerator::notStaticError(TypeAvailability t, SourcePosition p, const char *name) {
    if (!isStatic(t)) {
        throw CompilerError(p, "%s cannot be used dynamically.", name);
    }
}

std::pair<Type, TypeAvailability> CallableParserAndGenerator::parseTypeAsValue(TypeContext tc, SourcePosition p,
                                                                               Type expectation) {
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
        auto destination = Destination(DestinationMutability::Unknown, StorageType::Simple);
        auto type = parse(stream_.consumeToken(), destination);
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
            writer.writeInstruction(INS_GET_CLASS_FROM_INDEX, p);
            writer.writeInstruction(ot.eclass()->index, p);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndAvailabale);
        case TypeContent::Self:
            if (mode != CallableParserAndGeneratorMode::ClassMethod) {
                throw CompilerError(p, "Illegal use of 🐕.");
            }
            writer.writeInstruction(INS_GET_THIS, p);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::DynamicAndAvailabale);
        case TypeContent::LocalReference:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
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
    if (hasInstanceScope()) {
        scoper.instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired());
    }
    try {
        Scope &methodScope = scoper.pushScope();
        for (auto variable : callable.arguments) {
            auto &var = methodScope.setLocalVariable(variable.name.value(), variable.type, true,
                                                     variable.name.position());
            var.initialize(writer.writtenInstructions());
        }

        if (isFullyInitializedCheckRequired()) {
            auto initializer = static_cast<Initializer &>(callable);
            for (auto &var : initializer.argumentsToVariables()) {
                if (scoper.instanceScope()->hasLocalVariable(var) == 0) {
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
                produceToVariable(ResolvedVariable(instanceVariable, true), initializer.position());
                copyVariableContent(ResolvedVariable(argumentVariable, false), initializer.position());
            }
        }

        while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
            parseStatement(stream_.consumeToken());

            if (returned && !stream_.nextTokenIs(E_WATERMELON)) {
                compilerWarning(stream_.consumeToken(), "Dead code.");
                break;
            }
        }

        scoper.popScopeAndRecommendFrozenVariables(static_cast<Function &>(callable).objectVariableInformation(),
                                                   writer.writtenInstructions());

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
            if (!calledSuper && typeContext.calleeType().eclass()->superclass()) {
                throw CompilerError(initializer.position(),
                                             "Missing call to superinitializer in initializer %s.",
                                             initializer.name().utf8().c_str());
            }
        }
    }
    catch (CompilerError &ce) {
        printError(ce);
    }
}

void CallableParserAndGenerator::writeAndAnalyzeFunction(Function *function, CallableWriter &writer, Type contextType,
                                                         CallableScoper &scoper, CallableParserAndGeneratorMode mode) {
    if (contextType.type() == TypeContent::ValueType) {
        if (function->mutating()) {
            contextType.setMutable(true);
        }
        contextType.setValueReference();
    }
    auto sca = CallableParserAndGenerator(*function, function->package(), mode, TypeContext(contextType, function),
                                          writer, scoper);
    sca.analyze();
    function->setFullSize(scoper.fullSize());
}
