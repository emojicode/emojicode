//
//  FunctionPAG.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/01/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "FunctionPAG.hpp"
#include "../EmojicodeInstructions.h"
#include "BoxingLayer.hpp"
#include "CapturingCallableScoper.hpp"
#include "Class.hpp"
#include "CommonTypeFinder.hpp"
#include "Enum.hpp"
#include "ExpressionPAGs.hpp"
#include "Function.hpp"
#include "Protocol.hpp"
#include "RecompilationPoint.hpp"
#include "StringPool.hpp"
#include "Type.hpp"
#include "ValueType.hpp"
#include "VariableNotFoundError.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace EmojicodeCompiler {

Type FunctionPAG::parseTypeSafeExpr(Type type, std::vector<CommonTypeFinder> *ctargs) {
    auto token = stream_.consumeToken();
    auto returnType = ctargs ? parseExprToken(token, TypeExpectation(type.isReference(), type.requiresBox(), false)) :
                               parseExprToken(token, TypeExpectation(type));
    if (!returnType.compatibleTo(type, typeContext_, ctargs)) {
        auto cn = returnType.toString(typeContext_, true);
        auto tn = type.toString(typeContext_, true);
        throw CompilerError(token.position(), "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

bool FunctionPAG::typeIsEnumerable(const Type &type, Type *elementType) {
    if (type.type() == TypeContent::Class && !type.optional()) {
        for (Class *a = type.eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.protocol() == PR_ENUMERATEABLE) {
                    auto itemType = Type(TypeContent::GenericVariable, false, 0, PR_ENUMERATEABLE);
                    *elementType = itemType.resolveOn(protocol.resolveOn(type));
                    return true;
                }
            }
        }
    }
    else if (type.canHaveProtocol() && !type.optional()) {
        for (auto &protocol : type.typeDefinitionFunctional()->protocols()) {
            if (protocol.protocol() == PR_ENUMERATEABLE) {
                auto itemType = Type(TypeContent::GenericVariable, false, 0, PR_ENUMERATEABLE);
                *elementType = itemType.resolveOn(protocol.resolveOn(type));
                return true;
            }
        }
    }
    else if (type.type() == TypeContent::Protocol && type.protocol() == PR_ENUMERATEABLE) {
        *elementType = Type(TypeContent::GenericVariable, false, 0, type.protocol()).resolveOn(type);
        return true;
    }
    return false;
}

void FunctionPAG::checkAccessLevel(Function *function, const SourcePosition &p) const {
    if (function->accessLevel() == AccessLevel::Private) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || function->owningType().typeDefinition() != typeContext_.calleeType().typeDefinition()) {
            throw CompilerError(p, "%s is 🔒.", function->name().utf8().c_str());
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || !this->typeContext_.calleeType().eclass()->inheritsFrom(function->owningType().eclass())) {
            throw CompilerError(p, "%s is 🔐.", function->name().utf8().c_str());
        }
    }
}

Type FunctionPAG::parseFunctionCall(const Type &type, Function *p, const Token &token) {
    std::vector<Type> genericArguments;
    while (stream_.consumeTokenIf(E_SPIRAL_SHELL)) {
        genericArguments.push_back(parseTypeDeclarative(typeContext_, TypeDynamism::AllKinds));
    }

    auto inferGenericArguments = genericArguments.empty() && !p->genericArgumentVariables.empty();
    auto typeContext = TypeContext(type, p, inferGenericArguments ? nullptr : &genericArguments);

    if (inferGenericArguments) {
        std::vector<CommonTypeFinder> genericArgsFinders;
        genericArgsFinders.resize(p->genericArgumentVariables.size());

        auto recompilationPoint = RecompilationPoint(writer_, stream_);
        for (auto var : p->arguments) {
            auto resolved = var.type.resolveOn(typeContext);
            parseTypeSafeExpr(resolved, &genericArgsFinders);
        }

        typeContext = TypeContext(type, p, &genericArguments);

        for (size_t i = 0; i < genericArgsFinders.size(); i++) {
            auto commonType = genericArgsFinders[i].getCommonType(token.position());
            if (!commonType.compatibleTo(p->genericArgumentConstraints[i], typeContext)) {
                throw CompilerError(token.position(),
                                    "Infered type %s for generic argument %d is not compatible to constraint %s.",
                                    commonType.toString(typeContext, true).c_str(), i + 1,
                                    p->genericArgumentConstraints[i].toString(typeContext, true).c_str());
            }
            genericArguments.push_back(commonType);
        }

        recompilationPoint.restore();
    }
    else {
        if (genericArguments.size() != p->genericArgumentVariables.size()) {
            throw CompilerError(token.position(), "Too few generic arguments provided.");
        }
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
        writer_.writeInstruction(resolved.size());
        parseTypeSafeExpr(resolved);
    }

    p->deprecatedWarning(token.position());
    checkAccessLevel(p, token.position());
    return p->returnType.resolveOn(typeContext);
}

void FunctionPAG::writeInstructionForStackOrInstance(bool inInstanceScope, EmojicodeInstruction stack,
                                                                    EmojicodeInstruction object,
                                                                    EmojicodeInstruction vt) {
    if (!inInstanceScope) {
        writer_.writeInstruction(stack);
    }
    else {
        writer_.writeInstruction(typeContext_.calleeType().type() == TypeContent::ValueType ? vt : object);
        usedSelf = true;
    }
}

void FunctionPAG::produceToVariable(ResolvedVariable var) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_PRODUCE_WITH_STACK_DESTINATION,
                                       INS_PRODUCE_WITH_OBJECT_DESTINATION, INS_PRODUCE_WITH_VT_DESTINATION);
    writer_.writeInstruction(var.variable.id());
}

void FunctionPAG::getVTReference(ResolvedVariable var) {
    writeInstructionForStackOrInstance(var.inInstanceScope, INS_GET_VT_REFERENCE_STACK, INS_GET_VT_REFERENCE_OBJECT,
                                       INS_GET_VT_REFERENCE_VT);
    writer_.writeInstruction(var.variable.id());
}

void FunctionPAG::copyVariableContent(ResolvedVariable var) {
    if (var.variable.type().size() == 1) {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_SINGLE_STACK, INS_COPY_SINGLE_OBJECT,
                                           INS_COPY_SINGLE_VT);
        writer_.writeInstruction(var.variable.id());
    }
    else {
        writeInstructionForStackOrInstance(var.inInstanceScope, INS_COPY_WITH_SIZE_STACK, INS_COPY_WITH_SIZE_OBJECT,
                                           INS_COPY_WITH_SIZE_VT);
        writer_.writeInstruction(var.variable.id());
        writer_.writeInstruction(var.variable.type().size());
    }
}

void FunctionPAG::flowControlBlock(bool pushScope, const std::function<void()> &bodyPredicate) {
    if (pushScope) {
        scoper_.pushScope();
    }

    stream_.requireIdentifier(E_GRAPES);

    if (bodyPredicate) {
        bodyPredicate();
    }
    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        parseStatement();
        scoper_.clearTemporaryScope();
    }
    stream_.consumeToken();

    effect = true;

    scoper_.popScope(writer_.count());
}

void FunctionPAG::box(const TypeExpectation &expectation, Type &rtype, WriteLocation location) const {
    if (rtype.type() == TypeContent::Nothingness) {
        return;
    }
    if (!expectation.shouldPerformBoxing()) {
        return;
    }

    if (expectation.type() == TypeContent::Callable && rtype.type() == TypeContent::Callable &&
        expectation.genericArguments().size() == rtype.genericArguments().size()) {
        auto mismatch = std::mismatch(expectation.genericArguments().begin(), expectation.genericArguments().end(),
                                      rtype.genericArguments().begin(), [](const Type &a, const Type &b) {
                                          return a.storageType() == b.storageType();
                                      });
        if (mismatch.first != expectation.genericArguments().end()) {
            auto arguments = std::vector<Argument>();
            arguments.reserve(expectation.genericArguments().size() - 1);
            for (auto argumentType = expectation.genericArguments().begin() + 1;
                 argumentType != expectation.genericArguments().end(); argumentType++) {
                arguments.emplace_back(EmojicodeString(expectation.genericArguments().end() - argumentType),
                                       *argumentType);
            }
            auto destinationArgTypes = std::vector<Type>(rtype.genericArguments().begin() + 1,
                                                         rtype.genericArguments().end());
            auto boxingLayer = new BoxingLayer(destinationArgTypes, rtype.genericArguments()[0],
                                               function_.package(), arguments,
                                               expectation.genericArguments()[0], function_.position());
            function_.package()->registerFunction(boxingLayer);
            location.write({ INS_CLOSURE_BOX, static_cast<EmojicodeInstruction>(boxingLayer->vtiForUse()) });
        }
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
                    if (rtype.remotelyStored()) {
                        location.write({ INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE_REMOTE,
                            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
                    }
                    else {
                        location.write({ INS_BOX_TO_SIMPLE_OPTIONAL_PRODUCE,
                            static_cast<EmojicodeInstruction>(rtype.size()) });
                    }
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
                    if (rtype.remotelyStored()) {
                        location.write({ INS_SIMPLE_OPTIONAL_TO_BOX_REMOTE,
                            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()),
                            static_cast<EmojicodeInstruction>(rtype.size() - 1) });
                    }
                    else {
                        location.write({ INS_SIMPLE_OPTIONAL_TO_BOX,
                            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()) });
                    }
                    rtype.forceBox();
                    break;
                case StorageType::Simple:
                    if (rtype.remotelyStored()) {
                        location.write({ INS_BOX_PRODUCE_REMOTE,
                            static_cast<EmojicodeInstruction>(rtype.boxIdentifier()),
                            static_cast<EmojicodeInstruction>(rtype.size()) });
                    }
                    else {
                        location.write({ INS_BOX_PRODUCE, static_cast<EmojicodeInstruction>(rtype.boxIdentifier()) });
                    }
                    rtype.forceBox();
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
                    if (rtype.remotelyStored()) {
                        location.write({ INS_UNBOX_REMOTE, static_cast<EmojicodeInstruction>(rtype.size()) });
                    }
                    else {
                        location.write({ INS_UNBOX, static_cast<EmojicodeInstruction>(rtype.size()) });
                    }
                    break;
            }
            break;
    }

    if (!rtype.isReference() && expectation.isReference() && rtype.isReferencable()) {
        scoper_.pushTemporaryScope();
        auto &variable = scoper_.currentScope().setLocalInternalVariable(rtype, function_.position());
        insertionPoint.insert({ INS_PRODUCE_TO_AND_GET_VT_REFERENCE, EmojicodeInstruction(variable.id()) });
        variable.initialize(writer_.count());
        rtype.setReference();
    }
    else if (rtype.isReference() && !expectation.isReference()) {
        rtype.setReference(false);
        Type ptype = rtype;
        ptype.unbox();
        writer_.writeInstruction({ INS_COPY_FROM_REFERENCE, static_cast<EmojicodeInstruction>(ptype.size()) });
    }
}

FunctionWriter FunctionPAG::parseCondition(const Token &token, bool temporaryWriter) {
    auto writer = FunctionWriter();
    if (temporaryWriter) {
        std::swap(writer_, writer);
    }
    if (stream_.consumeTokenIf(E_SOFT_ICE_CREAM)) {
        auto placeholder = writer_.writeInstructionPlaceholder();

        auto &varName = stream_.consumeToken(TokenType::Variable);
        if (scoper_.currentScope().hasLocalVariable(varName.value())) {
            throw CompilerError(token.position(), "Cannot redeclare variable.");
        }

        Type t = parseExpr(TypeExpectation(true, false));
        if (!t.optional()) {
            throw CompilerError(token.position(), "Condition assignment can only be used with optionals.");
        }

        auto box = t.storageType() == StorageType::Box;
        placeholder.write(box ? INS_CONDITIONAL_PRODUCE_BOX : INS_CONDITIONAL_PRODUCE_SIMPLE_OPTIONAL);

        t.setReference(false);
        t.setOptional(false);

        scoper_.clearTemporaryScope();
        auto &variable = scoper_.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
        variable.initialize(writer_.count());
        writer_.writeInstruction(variable.id());
        if (!box) {
            writer_.writeInstruction(t.size());
        }
    }
    else {
        parseTypeSafeExpr(Type::boolean());
    }
    if (temporaryWriter) {
        std::swap(writer_, writer);
    }
    scoper_.clearTemporaryScope();
    return writer;
}

bool FunctionPAG::isSuperconstructorRequired() const {
    return function_.compilationMode() == FunctionPAGMode::ObjectInitializer;
}

bool FunctionPAG::isFullyInitializedCheckRequired() const {
    return function_.compilationMode() == FunctionPAGMode::ObjectInitializer ||
    function_.compilationMode() == FunctionPAGMode::ValueTypeInitializer;
}

bool FunctionPAG::isSelfAllowed() const {
    return function_.compilationMode() != FunctionPAGMode::Function &&
    function_.compilationMode() != FunctionPAGMode::ClassMethod;
}

bool FunctionPAG::hasInstanceScope() const {
    return hasInstanceScope(function_.compilationMode());
}

bool FunctionPAG::hasInstanceScope(FunctionPAGMode mode) {
    return (mode == FunctionPAGMode::ObjectMethod ||
            mode == FunctionPAGMode::ObjectInitializer ||
            mode == FunctionPAGMode::ValueTypeInitializer ||
            mode == FunctionPAGMode::ValueTypeMethod);
}

bool FunctionPAG::isOnlyNothingnessReturnAllowed() const {
    return function_.compilationMode() == FunctionPAGMode::ObjectInitializer ||
    function_.compilationMode() == FunctionPAGMode::ValueTypeInitializer;
}

void FunctionPAG::mutatingMethodCheck(Function *method, const Type &type, const TypeExpectation &expectation,
                                      const SourcePosition &p) {
    if (method->mutating()) {
        if (!type.isMutable()) {
            throw CompilerError(p, "%s was marked 🖍 but callee is not mutable.", method->name().utf8().c_str());
        }
        expectation.mutateOriginVariable(p);
    }
}

void FunctionPAG::parseStatement() {
    const Token &token = stream_.consumeToken();
    if (token.type() == TokenType::Identifier) {
        switch (token.value()[0]) {
            case E_SHORTCAKE: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                Type t = parseTypeDeclarative(typeContext_, TypeDynamism::AllKinds);

                auto &var = scoper_.currentScope().setLocalVariable(varName.value(), t, false, varName.position());

                if (t.optional()) {
                    var.initialize(writer_.count());
                    writer_.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);
                    writer_.writeInstruction(var.id());
                    writer_.writeInstruction(INS_GET_NOTHINGNESS);
                }
                return;
            }
            case E_CUSTARD: {
                auto &nextTok = stream_.consumeToken();

                if (nextTok.type() == TokenType::Identifier) {
                    auto &varName = stream_.consumeToken(TokenType::Variable);

                    auto var = scoper_.getVariable(varName.value(), varName.position());

                    var.variable.uninitalizedError(varName.position());
                    var.variable.mutate(varName.position());

                    produceToVariable(var);

                    if (var.variable.type().type() == TypeContent::ValueType &&
                        var.variable.type().valueType() == VT_INTEGER && stream_.nextTokenIs(TokenType::Integer)
                        && stream_.nextToken().value() == EmojicodeString('1')) {
                        if (nextTok.value()[0] == E_HEAVY_PLUS_SIGN) {
                            writer_.writeInstruction(INS_INCREMENT);
                            stream_.consumeToken();
                            return;
                        }
                        if (nextTok.value()[0] == E_HEAVY_MINUS_SIGN) {
                            writer_.writeInstruction(INS_DECREMENT);
                            stream_.consumeToken();
                            return;
                        }
                    }

                    auto type = parseMethodCall(nextTok, TypeExpectation(var.variable.type()),
                                                [this, var, &nextTok](const TypeExpectation &expectation) {
                        takeVariable(var, expectation);
                        return var.variable.type();
                    });
                    if (!type.compatibleTo(var.variable.type(), typeContext_)) {
                        throw CompilerError(nextTok.position(), "Method %s returns incompatible type %s.",
                                            nextTok.value().utf8().c_str(), type.toString(typeContext_, true).c_str());
                    }
                }
                else {
                    Type type = Type::nothingness();
                    try {
                        auto rvar = scoper_.getVariable(nextTok.value(), nextTok.position());

                        rvar.variable.initialize(writer_.count());
                        rvar.variable.mutate(nextTok.position());

                        produceToVariable(rvar);

                        if (rvar.inInstanceScope && !typeContext_.calleeType().isMutable()) {
                            throw CompilerError(token.position(),
                                                "Can’t mutate instance variable as method is not marked with 🖍.");
                        }

                        type = rvar.variable.type();
                    }
                    catch (VariableNotFoundError &vne) {
                        // Not declared, declaring as local variable
                        writer_.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);

                        auto placeholder = writer_.writeInstructionPlaceholder();

                        Type t = parseExpr(TypeExpectation(false, true));
                        scoper_.clearTemporaryScope();
                        auto &var = scoper_.currentScope().setLocalVariable(nextTok.value(), t, false, nextTok.position());
                        var.initialize(writer_.count());
                        placeholder.write(var.id());
                        return;
                    }
                    parseTypeSafeExpr(type);
                }
                return;
            }
            case E_SOFT_ICE_CREAM: {
                auto &varName = stream_.consumeToken(TokenType::Variable);

                writer_.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);

                auto placeholder = writer_.writeInstructionPlaceholder();

                Type t = parseExpr(TypeExpectation(false, false));
                scoper_.clearTemporaryScope();
                auto &var = scoper_.currentScope().setLocalVariable(varName.value(), t, true, varName.position());
                var.initialize(writer_.count());
                placeholder.write(var.id());
                return;
            }
            case E_TANGERINE: {
                // This list will contain all placeholders that need to be replaced with a relative jump value
                // to jump past the whole if
                auto endPlaceholders = std::vector<std::pair<InstructionCount, FunctionWriterPlaceholder>>();

                std::experimental::optional<FunctionWriterCountPlaceholder> placeholder;
                do {
                    pathAnalyser.beginBranch();
                    if (placeholder) {
                        writer_.writeInstruction(INS_JUMP_FORWARD);
                        auto endPlaceholder = writer_.writeInstructionPlaceholder();
                        endPlaceholders.emplace_back(writer_.count(), endPlaceholder);
                        placeholder->write();
                    }

                    writer_.writeInstruction(INS_JUMP_FORWARD_IF_NOT);
                    scoper_.pushScope();
                    parseCondition(token, false);
                    placeholder = writer_.writeInstructionsCountPlaceholderCoin();
                    flowControlBlock(false);
                    pathAnalyser.endBranch();
                } while (stream_.consumeTokenIf(E_LEMON));

                if (stream_.consumeTokenIf(E_STRAWBERRY)) {
                    pathAnalyser.beginBranch();
                    // Jump past the else block if the last if evaluated to true, this line is skipped above for true
                    writer_.writeInstruction(INS_JUMP_FORWARD);
                    auto elseCountPlaceholder = writer_.writeInstructionsCountPlaceholderCoin();
                    // Jump into the else block if the last if evaluated to false
                    placeholder->write();
                    flowControlBlock();
                    elseCountPlaceholder.write();
                    pathAnalyser.endBranch();
                    pathAnalyser.endMutualExclusiveBranches();
                }
                else {
                    placeholder->write();
                    // If there’s no else clause we can throw away the incidents right away as none of them could have
                    // been executed and nothing can be concluded
                    pathAnalyser.endUncertainBranches();
                }

                for (auto endPlaceholder : endPlaceholders) {
                    endPlaceholder.second.write(writer_.count() - endPlaceholder.first);
                }
                return;
            }
            case E_AVOCADO: {
                auto &variableToken = stream_.consumeToken(TokenType::Variable);

                writer_.writeInstruction(INS_PRODUCE_WITH_STACK_DESTINATION);
                auto placeholder = writer_.writeInstructionPlaceholder();

                Type type = parseExpr(TypeExpectation(false, false));

                if (type.type() != TypeContent::Error) {
                    throw CompilerError(token.position(), "🥑 can only be used with 🚨.");
                }

                auto &wrscope = scoper_.pushScope();
                int variableID = wrscope.setLocalInternalVariable(type, variableToken.position()).id();
                placeholder.write(variableID);
                // Intentionally not initializing. The GC can not be invoked between here and the initialization of
                // the user-facing variables below.

                auto &scope = scoper_.pushScope();

                if (type.storageType() == StorageType::Box) {
                    Type vtype = type.genericArguments()[1];
                    vtype.forceBox();
                    scope.setLocalVariableWithID(variableToken.value(), vtype, true, variableID,
                                                 variableToken.position()).initialize(writer_.count());

                }
                else if (type.genericArguments()[1].storageType() == StorageType::SimpleOptional) {
                    scope.setLocalVariableWithID(variableToken.value(), type.genericArguments()[1], true, variableID,
                                                 variableToken.position()).initialize(writer_.count());
                }
                else {
                    scope.setLocalVariableWithID(variableToken.value(), type.genericArguments()[1], true,
                                                 variableID + 1, variableToken.position())
                                                 .initialize(writer_.count());
                }

                writer_.writeInstruction(INS_JUMP_FORWARD_IF);
                writer_.writeInstruction(INS_IS_ERROR);
                writer_.writeInstruction(INS_GET_VT_REFERENCE_STACK);
                writer_.writeInstruction(variableID);
                auto countPlaceholder = writer_.writeInstructionsCountPlaceholderCoin();
                flowControlBlock(false);
                writer_.writeInstruction(INS_JUMP_FORWARD);
                auto skipElsePlaceholder = writer_.writeInstructionsCountPlaceholderCoin();
                countPlaceholder.write();

                stream_.requireIdentifier(E_STRAWBERRY);
                auto &errorVariableToken = stream_.consumeToken(TokenType::Variable);

                auto &errorScope = scoper_.pushScope();
                errorScope.setLocalVariableWithID(errorVariableToken.value(), type.genericArguments()[0], true,
                                                  variableID + 1, errorVariableToken.position())
                                                  .initialize(writer_.count());
                flowControlBlock(false);
                skipElsePlaceholder.write();

                scoper_.popScope(writer_.count());
                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS: {
                writer_.writeInstruction(INS_JUMP_FORWARD);
                auto placeholder = writer_.writeInstructionsCountPlaceholderCoin();

                auto conditionWriter = parseCondition(token, true);
                auto delta = writer_.count();

                pathAnalyser.beginBranch();

                flowControlBlock();
                placeholder.write();

                writer_.writeInstruction(INS_JUMP_BACKWARD_IF);
                writer_.writeWriter(conditionWriter);
                writer_.writeInstruction(writer_.count() - delta + 1);

                pathAnalyser.endBranch();
                pathAnalyser.endUncertainBranches();
                return;
            }
            case E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY: {
                auto &variableToken = stream_.consumeToken(TokenType::Variable);
                auto &iteratorScope = scoper_.pushScope();
                auto &iteratorVar = iteratorScope.setLocalVariable(EmojicodeString(), Type(PR_ENUMERATOR, false),
                                                                   true, token.position());

                auto insertionPoint = writer_.getInsertionPoint();
                Type iteratee = parseExpr(TypeExpectation());

                Type itemType = Type::nothingness();
                if (!typeIsEnumerable(iteratee, &itemType)) {
                    auto iterateeString = iteratee.toString(typeContext_, true);
                    throw CompilerError(token.position(), "%s does not conform to s🔂.", iterateeString.c_str());
                }

                auto iteratorMethodIndex = PR_ENUMERATEABLE->lookupMethod(EmojicodeString(E_DANGO))->vtiForUse();
                auto nextVTI = PR_ENUMERATOR->lookupMethod(EmojicodeString(E_DOWN_POINTING_SMALL_RED_TRIANGLE))->vtiForUse();
                auto moreVTI = PR_ENUMERATOR->lookupMethod(EmojicodeString(E_RED_QUESTION_MARK))->vtiForUse();

                insertionPoint.insert({ INS_PRODUCE_WITH_STACK_DESTINATION,
                    static_cast<EmojicodeInstruction>(iteratorVar.id()), INS_DISPATCH_PROTOCOL,
                });
                box(TypeExpectation(true, true, false), iteratee, insertionPoint);
                iteratorVar.initialize(writer_.count());

                scoper_.clearTemporaryScope();

                writer_.writeInstruction({ static_cast<EmojicodeInstruction>(PR_ENUMERATEABLE->index),
                    static_cast<EmojicodeInstruction>(iteratorMethodIndex)
                });
                writer_.writeInstruction(INS_JUMP_FORWARD);
                auto placeholder = writer_.writeInstructionsCountPlaceholderCoin();
                auto delta = writer_.count();

                scoper_.pushScope();
                auto &var = scoper_.currentScope().setLocalVariable(variableToken.value(), itemType, true,
                                                                    variableToken.position());

                pathAnalyser.beginBranch();

                flowControlBlock(false, [this, iteratorVar, &var, nextVTI]{
                    var.initialize(writer_.count());
                    writer_.writeInstruction({ INS_PRODUCE_WITH_STACK_DESTINATION,
                        static_cast<EmojicodeInstruction>(var.id()), INS_DISPATCH_PROTOCOL,
                        INS_GET_VT_REFERENCE_STACK,
                        static_cast<EmojicodeInstruction>(iteratorVar.id()),
                        static_cast<EmojicodeInstruction>(PR_ENUMERATOR->index),
                        static_cast<EmojicodeInstruction>(nextVTI)
                    });
                });
                placeholder.write();
                writer_.writeInstruction({ INS_JUMP_BACKWARD_IF, INS_DISPATCH_PROTOCOL,
                    INS_GET_VT_REFERENCE_STACK, static_cast<EmojicodeInstruction>(iteratorVar.id()),
                    static_cast<EmojicodeInstruction>(PR_ENUMERATOR->index),
                    static_cast<EmojicodeInstruction>(moreVTI) });
                writer_.writeInstruction(writer_.count() - delta + 1);
                scoper_.popScope(writer_.count());

                pathAnalyser.endBranch();
                pathAnalyser.endUncertainBranches();
                return;
            }
            case E_GOAT: {
                if (!isSuperconstructorRequired()) {
                    throw CompilerError(token.position(), "🐐 can only be used inside initializers.");
                }
                if (typeContext_.calleeType().eclass()->superclass() == nullptr) {
                    throw CompilerError(token.position(), "🐐 can only be used if the eclass inherits from another.");
                }
                if (pathAnalyser.hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
                    throw CompilerError(token.position(), "Superinitializer might have already been called.");
                }

                scoper_.instanceScope()->initializerUnintializedVariablesCheck(token.position(),
                                                                              "Instance variable \"%s\" must be "
                                                                              "initialized before superinitializer.");

                writer_.writeInstruction(INS_SUPER_INITIALIZER);

                Class *eclass = typeContext_.calleeType().eclass();

                writer_.writeInstruction(INS_GET_CLASS_FROM_INDEX);
                writer_.writeInstruction(eclass->superclass()->index);

                auto &initializerToken = stream_.consumeToken(TokenType::Identifier);

                auto initializer = eclass->superclass()->getInitializer(initializerToken, Type(eclass, false), typeContext_);

                writer_.writeInstruction(initializer->vtiForUse());

                parseFunctionCall(typeContext_.calleeType(), initializer, token);

                pathAnalyser.recordIncident(PathAnalyserIncident::CalledSuperInitializer);
                return;
            }
            case E_POLICE_CARS_LIGHT: {
                writer_.writeInstruction(INS_ERROR);

                if (isOnlyNothingnessReturnAllowed()) {
                    auto &initializer = static_cast<Initializer &>(function_);
                    if (!initializer.errorProne()) {
                        throw CompilerError(token.position(), "Initializer is not declared error-prone.");
                    }
                    parseTypeSafeExpr(initializer.errorType());
                    pathAnalyser.recordIncident(PathAnalyserIncident::Returned);
                    return;
                }
                if (function_.returnType.type() != TypeContent::Error) {
                    throw CompilerError(token.position(), "Function is not declared to return a 🚨.");
                }

                parseTypeSafeExpr(function_.returnType.genericArguments()[0]);
                pathAnalyser.recordIncident(PathAnalyserIncident::Returned);
                return;
            }
            case E_RED_APPLE: {
                if (function_.returnType.type() == TypeContent::Nothingness) {
                    pathAnalyser.recordIncident(PathAnalyserIncident::Returned);
                    writer_.writeInstruction(INS_RETURN_WITHOUT_VALUE);
                    return;
                }

                writer_.writeInstruction(INS_RETURN);

                if (isOnlyNothingnessReturnAllowed()) {
                    throw CompilerError(token.position(), "🍎 cannot be used inside an initializer.");
                }

                parseTypeSafeExpr(function_.returnType);
                pathAnalyser.recordIncident(PathAnalyserIncident::Returned);
                return;
            }
        }
    }

    // Must be an expression
    effect = false;
    auto type = parseExprToken(token, TypeExpectation(true, false, false));
    noEffectWarning(token);
}

Type FunctionPAG::parseExprToken(const Token &token, TypeExpectation &&expectation) {
    switch (token.type()) {
        case TokenType::String: {
            box(expectation, Type(CL_STRING, false));
            writer_.writeInstruction(INS_GET_STRING_POOL);
            writer_.writeInstruction(StringPool::theStringPool().poolString(token.value()));
            return Type(CL_STRING, false);
        }
        case TokenType::BooleanTrue:
            box(expectation, Type::boolean());
            writer_.writeInstruction(INS_GET_TRUE);
            return Type::boolean();
        case TokenType::BooleanFalse:
            box(expectation, Type::boolean());
            writer_.writeInstruction(INS_GET_FALSE);
            return Type::boolean();
        case TokenType::Integer: {
            long long value = std::stoll(token.value().utf8(), nullptr, 0);

            if (expectation.type() == TypeContent::ValueType && expectation.valueType() == VT_DOUBLE) {
                box(expectation, Type::doubl());
                writer_.writeInstruction(INS_GET_DOUBLE);
                writer_.writeDoubleCoin(value);
                return Type::doubl();
            }

            if (std::llabs(value) > INT32_MAX) {
                box(expectation, Type::integer());
                writer_.writeInstruction(INS_GET_64_INTEGER);

                writer_.writeInstruction(value >> 32);
                writer_.writeInstruction(static_cast<EmojicodeInstruction>(value));

                return Type::integer();
            }

            box(expectation, Type::integer());
            writer_.writeInstruction(INS_GET_32_INTEGER);
            value += INT32_MAX;
            writer_.writeInstruction(static_cast<EmojicodeInstruction>(value));

            return Type::integer();
        }
        case TokenType::Double: {
            box(expectation, Type::doubl());
            writer_.writeInstruction(INS_GET_DOUBLE);

            double d = std::stod(token.value().utf8());
            writer_.writeDoubleCoin(d);

            return Type::doubl();
        }
        case TokenType::Symbol:
            box(expectation, Type::symbol());
            writer_.writeInstruction(INS_GET_SYMBOL);
            writer_.writeInstruction(token.value()[0]);
            return Type::symbol();
        case TokenType::Variable: {
            auto rvar = scoper_.getVariable(token.value(), token.position());
            rvar.variable.uninitalizedError(token.position());

            expectation.setOriginVariable(rvar.variable);
            return takeVariable(rvar, expectation);
        }
        case TokenType::Identifier:
            return parseExprIdentifier(token, expectation);
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
        expectation.isReference() && rvar.variable.type().isReferencable()) {
        getVTReference(rvar);
        returnType.setReference();
    }
    else {
        box(expectation, returnType, writer_);
        copyVariableContent(rvar);
        if (expectation.isMutable()) {
            returnType.setMutable(true);
        }
    }
    return returnType;
}

Type FunctionPAG::parseExprIdentifier(const Token &token, const TypeExpectation &expectation) {
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
        case E_COOKIE:
            return pagConcatenateLiteral(token, expectation, *this);
        case E_ICE_CREAM:
            return pagListLiteral(token, expectation, *this);
        case E_HONEY_POT:
            return pagDictionaryLiteral(token, expectation, *this);
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE:
        case E_BLACK_RIGHT_POINTING_DOUBLE_TRIANGLE_WITH_VERTICAL_BAR:
            return pagRangeLiteral(token, expectation, *this);
        case E_DOG: {
            if (isSuperconstructorRequired() && !pathAnalyser.hasCertainly(PathAnalyserIncident::CalledSuperInitializer) &&
                function_.owningType().eclass()->superclass() != nullptr) {
                throw CompilerError(token.position(), "Attempt to use 🐕 before superinitializer call.");
            }
            if (isFullyInitializedCheckRequired()) {
                scoper_.instanceScope()->initializerUnintializedVariablesCheck(token.position(),
                                                                              "Instance variable \"%s\" must be "
                                                                              "initialized before the use of 🐕.");
            }

            if (!isSelfAllowed()) {
                throw CompilerError(token.position(), "Illegal use of 🐕.");
            }

            auto type = typeContext_.calleeType();
            box(expectation, type, writer_);
            usedSelf = true;
            writer_.writeInstruction(INS_THIS);

            return type;
        }
        case E_HIGH_VOLTAGE_SIGN: {
            writer_.writeInstruction(INS_GET_NOTHINGNESS);
            return Type::nothingness();
        }
        case E_CLOUD:
            return pagIsNothingness(token, expectation, *this);
        case E_TRAFFIC_LIGHT:
            return pagIsError(token, expectation, *this);
        case E_FACE_WITH_STUCK_OUT_TONGUE_AND_WINKING_EYE:
            return pagIdentityCheck(token, expectation, *this);
        case E_WHITE_LARGE_SQUARE:
            return pagTypeInstanceFromInstance(token, expectation, *this);
        case E_WHITE_SQUARE_BUTTON:
            return pagTypeInstance(token, expectation, *this);
        case E_BLACK_SQUARE_BUTTON:
            return pagCast(token, expectation, *this);
        case E_BEER_MUG:
            return pagUnwrapOptional(token, expectation, *this);
        case E_METRO:
            return pagExtraction(token, expectation, *this);
        case E_HOT_PEPPER:
            return pagMethodCapture(token, expectation, *this);
        case E_GRAPES: {
            auto function = new Function(EmojicodeString(E_GRAPES), AccessLevel::Public, true, function_.owningType(),
                                         package_, token.position(), false, EmojicodeString(), false, false,
                                         function_.compilationMode());
            parseArgumentList(function, typeContext_, true);
            parseReturnType(function, typeContext_);
            parseBodyBlock(function);

            function->setVtiProvider(&Function::pureFunctionsProvider);
            package_->registerFunction(function);

            auto closureScoper = CapturingCallableScoper(scoper_);
            auto pag = FunctionPAG(*function, function->owningType(), function->writer_, closureScoper);
            pag.compile();

            auto insertionPoint = writer_.getInsertionPoint();
            writer_.writeInstruction(INS_CLOSURE);
            function->markUsed(false);
            writer_.writeInstruction(function->vtiForUse());
            writer_.writeInstruction(static_cast<EmojicodeInstruction>(closureScoper.captures().size()));
            writer_.writeInstruction(closureScoper.captureSize());

            auto objectVariableInformation = std::vector<ObjectVariableInformation>();
            objectVariableInformation.reserve(closureScoper.captures().size());
            int index = 0;
            for (auto capture : closureScoper.captures()) {
                writer_.writeInstruction(capture.id);
                writer_.writeInstruction(capture.type.size());
                writer_.writeInstruction(capture.captureId);
                capture.type.objectVariableRecords(index, objectVariableInformation);
                index += capture.type.size();
            }
            writer_.writeInstruction(objectVariableInformation.size());
            for (auto &record : objectVariableInformation) {
                writer_.writeInstruction((record.conditionIndex << 16) | static_cast<uint16_t>(record.index));
                writer_.writeInstruction(static_cast<uint16_t>(record.type));
            }
            writer_.writeInstruction(pag.usedSelfInBody() ? 1 : 0);

            auto type = function->type();
            box(expectation, type, insertionPoint);
            return type;
        }
        case E_LOLLIPOP:
            return pagCallableCall(token, expectation, *this);
        case E_CHIPMUNK:
            return pagSuperMethod(token, expectation, *this);
        case E_LARGE_BLUE_DIAMOND:
            return pagInstatiation(token, expectation, *this);
        case E_DOUGHNUT:
            return pagTypeMethod(token, expectation, *this);
        default: {
            makeEffective();
            return parseMethodCall(token, expectation, [this](TypeExpectation &expectation) {
                return parseExpr(std::move(expectation));
            });
        }
    }
    return Type::nothingness();
}

Type FunctionPAG::parseMethodCall(const Token &token, const TypeExpectation &expectation,
                                  std::function<Type(TypeExpectation &)> callee) {
    auto insertionPoint = writer_.getInsertionPoint();
    auto placeholder = writer_.writeInstructionPlaceholder();

    auto recompilationPoint = RecompilationPoint(writer_, stream_);
    auto recompileWithSimple = [this, recompilationPoint, &callee]() {
        recompilationPoint.restore();
        auto simpleExpectation = TypeExpectation(false, false, false);
        callee(simpleExpectation);
    };
    auto writePrimitiveMethod = [this, &placeholder, &insertionPoint, &expectation,
                                 &recompileWithSimple](EmojicodeInstruction instruction, Type returnType,
                                 std::experimental::optional<Type> argument) {
        placeholder.write(instruction);
        recompileWithSimple();
        box(expectation, returnType, insertionPoint);
        if (argument) {
            parseTypeSafeExpr(*argument);
        }
        return returnType;
    };

    auto calleeExpectation = TypeExpectation(true, false);
    Type rtype = callee(calleeExpectation);
    Type type = rtype.resolveOnSuperArgumentsAndConstraints(typeContext_);

    if (type.optional()) {
        throw CompilerError(token.position(), "You cannot call methods on optionals.");
    }

    Function *method = nullptr;
    if (type.type() == TypeContent::ValueType) {
        if (type.valueType() == VT_BOOLEAN) {
            switch (token.value()[0]) {
                case E_NEGATIVE_SQUARED_CROSS_MARK:
                    return writePrimitiveMethod(INS_INVERT_BOOLEAN, Type::boolean(), std::experimental::nullopt);
                case E_PARTY_POPPER:
                    return writePrimitiveMethod(INS_OR_BOOLEAN, Type::boolean(), Type::boolean());
                case E_CONFETTI_BALL:
                    return writePrimitiveMethod(INS_AND_BOOLEAN, Type::boolean(), Type::boolean());
            }
        }
        else if (type.valueType() == VT_INTEGER) {
            switch (token.value()[0]) {
                case E_HEAVY_MINUS_SIGN:
                    return writePrimitiveMethod(INS_SUBTRACT_INTEGER, Type::integer(), Type::integer());
                case E_HEAVY_PLUS_SIGN:
                    return writePrimitiveMethod(INS_ADD_INTEGER, Type::integer(), Type::integer());
                case E_HEAVY_DIVISION_SIGN:
                    return writePrimitiveMethod(INS_DIVIDE_INTEGER, Type::integer(), Type::integer());
                case E_HEAVY_MULTIPLICATION_SIGN:
                    return writePrimitiveMethod(INS_MULTIPLY_INTEGER, Type::integer(), Type::integer());
                case E_LEFT_POINTING_TRIANGLE:
                    return writePrimitiveMethod(INS_LESS_INTEGER, Type::boolean(), Type::integer());
                case E_RIGHT_POINTING_TRIANGLE:
                    return writePrimitiveMethod(INS_GREATER_INTEGER, Type::boolean(), Type::integer());
                case E_LEFTWARDS_ARROW:
                    return writePrimitiveMethod(INS_LESS_OR_EQUAL_INTEGER, Type::boolean(), Type::integer());
                case E_RIGHTWARDS_ARROW:
                    return writePrimitiveMethod(INS_GREATER_OR_EQUAL_INTEGER, Type::boolean(), Type::integer());
                case E_PUT_LITTER_IN_ITS_SPACE:
                    return writePrimitiveMethod(INS_REMAINDER_INTEGER, Type::integer(), Type::integer());
                case E_HEAVY_LARGE_CIRCLE:
                    return writePrimitiveMethod(INS_BINARY_AND_INTEGER, Type::integer(), Type::integer());
                case E_ANGER_SYMBOL:
                    return writePrimitiveMethod(INS_BINARY_OR_INTEGER, Type::integer(), Type::integer());
                case E_CROSS_MARK:
                    return writePrimitiveMethod(INS_BINARY_XOR_INTEGER, Type::integer(), Type::integer());
                case E_NO_ENTRY_SIGN:
                    return writePrimitiveMethod(INS_BINARY_NOT_INTEGER, Type::integer(), std::experimental::nullopt);
                case E_ROCKET:
                    return writePrimitiveMethod(INS_INT_TO_DOUBLE, Type::doubl(), std::experimental::nullopt);
                case E_LEFT_POINTING_BACKHAND_INDEX:
                    return writePrimitiveMethod(INS_SHIFT_LEFT_INTEGER, Type::integer(), Type::integer());
                case E_RIGHT_POINTING_BACKHAND_INDEX:
                    return writePrimitiveMethod(INS_SHIFT_RIGHT_INTEGER, Type::integer(), Type::integer());
            }
        }
        else if (type.valueType() == VT_DOUBLE) {
            switch (token.value()[0]) {
                case E_FACE_WITH_STUCK_OUT_TONGUE:
                    return writePrimitiveMethod(INS_EQUAL_DOUBLE, Type::boolean(), Type::doubl());
                case E_HEAVY_MINUS_SIGN:
                    return writePrimitiveMethod(INS_SUBTRACT_DOUBLE, Type::doubl(), Type::doubl());
                case E_HEAVY_PLUS_SIGN:
                    return writePrimitiveMethod(INS_ADD_DOUBLE, Type::doubl(), Type::doubl());
                case E_HEAVY_DIVISION_SIGN:
                    return writePrimitiveMethod(INS_DIVIDE_DOUBLE, Type::doubl(), Type::doubl());
                case E_HEAVY_MULTIPLICATION_SIGN:
                    return writePrimitiveMethod(INS_MULTIPLY_DOUBLE, Type::doubl(), Type::doubl());
                case E_LEFT_POINTING_TRIANGLE:
                    return writePrimitiveMethod(INS_LESS_DOUBLE, Type::boolean(), Type::doubl());
                case E_RIGHT_POINTING_TRIANGLE:
                    return writePrimitiveMethod(INS_GREATER_DOUBLE, Type::boolean(), Type::doubl());
                case E_LEFTWARDS_ARROW:
                    return writePrimitiveMethod(INS_LESS_OR_EQUAL_DOUBLE, Type::boolean(), Type::doubl());
                case E_RIGHTWARDS_ARROW:
                    return writePrimitiveMethod(INS_GREATER_OR_EQUAL_DOUBLE, Type::boolean(), Type::doubl());
                case E_PUT_LITTER_IN_ITS_SPACE:
                    return writePrimitiveMethod(INS_REMAINDER_DOUBLE, Type::doubl(), Type::doubl());
            }
        }
        else if (type.valueType() == VT_SYMBOL && token.isIdentifier(E_FACE_WITH_STUCK_OUT_TONGUE)) {
            return writePrimitiveMethod(INS_EQUAL_SYMBOL, Type::boolean(), Type::symbol());
        }

        if (token.isIdentifier(E_FACE_WITH_STUCK_OUT_TONGUE) && type.valueType()->isPrimitive()) {
            recompileWithSimple();
            type.setReference(false);
            parseTypeSafeExpr(type);
            placeholder.write(INS_EQUAL_PRIMITIVE);

            return Type::boolean();
        }
    }
    else if (type.type() == TypeContent::Enum && token.value()[0] == E_FACE_WITH_STUCK_OUT_TONGUE) {
        recompileWithSimple();
        type.setReference(false);
        parseTypeSafeExpr(type);  // Must be of the same type as the callee
        placeholder.write(INS_EQUAL_PRIMITIVE);
        return Type::boolean();
    }

    if (type.type() == TypeContent::ValueType) {
        method = type.valueType()->getMethod(token, type, typeContext_);
        mutatingMethodCheck(method, type, calleeExpectation, token.position());
        placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
        writer_.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::Protocol) {
        method = type.protocol()->getMethod(token, type, typeContext_);
        placeholder.write(INS_DISPATCH_PROTOCOL);
        writer_.writeInstruction(type.protocol()->index);
        writer_.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::MultiProtocol) {
        for (auto &protocol : type.protocols()) {
            if ((method = protocol.protocol()->lookupMethod(token.value())) != nullptr) {
                placeholder.write(INS_DISPATCH_PROTOCOL);
                writer_.writeInstruction(protocol.protocol()->index);
                writer_.writeInstruction(method->vtiForUse());
                break;
            }
        }
        if (method == nullptr) {
            throw CompilerError(token.position(), "No type in %s provides a method called %s.",
                                type.toString(typeContext_, true).c_str(), token.value().utf8().c_str());
        }
    }
    else if (type.type() == TypeContent::Enum) {
        method = type.eenum()->getMethod(token, type, typeContext_);
        placeholder.write(INS_CALL_CONTEXTED_FUNCTION);
        writer_.writeInstruction(method->vtiForUse());
    }
    else if (type.type() == TypeContent::Class) {
        method = type.eclass()->getMethod(token, type, typeContext_);
        placeholder.write(INS_DISPATCH_METHOD);
        writer_.writeInstruction(method->vtiForUse());
    }
    else {
        auto typeString = type.toString(typeContext_, true);
        throw CompilerError(token.position(), "You cannot call methods on %s.", typeString.c_str());
    }

    Type rt = parseFunctionCall(type, method, token);
    box(expectation, rt, insertionPoint);
    return rt;
}

void FunctionPAG::noEffectWarning(const Token &warningToken) {
    if (!effect) {
        compilerWarning(warningToken.position(), "Statement seems to have no effect whatsoever.");
    }
}

std::pair<Type, TypeAvailability> FunctionPAG::parseTypeAsValue(const SourcePosition &p,
                                                                const TypeExpectation &expectation) {
    if (stream_.consumeTokenIf(E_BLACK_LARGE_SQUARE)) {
        auto type = parseExpr(TypeExpectation(false, false, false));
        if (!type.meta()) {
            throw CompilerError(p, "Expected meta type.");
        }
        if (type.optional()) {
            throw CompilerError(p, "🍬 can’t be used as meta type.");
        }
        type.setMeta(false);
        return std::pair<Type, TypeAvailability>(type, TypeAvailability::DynamicAndAvailabale);
    }
    Type ot = parseTypeDeclarative(typeContext_, TypeDynamism::AllKinds, expectation);
    switch (ot.type()) {
        case TypeContent::GenericVariable:
            throw CompilerError(p, "Generic Arguments are not yet available for reflection.");
        case TypeContent::Class:
            writer_.writeInstruction(INS_GET_CLASS_FROM_INDEX);
            writer_.writeInstruction(ot.eclass()->index);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndAvailabale);
        case TypeContent::Self:
            if (function_.compilationMode() != FunctionPAGMode::ClassMethod) {
                throw CompilerError(p, "Illegal use of 🐕.");
            }
            writer_.writeInstruction(INS_THIS);
            return std::pair<Type, TypeAvailability>(ot, TypeAvailability::DynamicAndAvailabale);
        case TypeContent::LocalGenericVariable:
            throw CompilerError(p, "Function Generic Arguments are not available for reflection.");
        default:
            break;
    }
    return std::pair<Type, TypeAvailability>(ot, TypeAvailability::StaticAndUnavailable);
}

Scope& FunctionPAG::setArgumentVariables() {
    Scope &methodScope = scoper_.pushScope();
    for (auto variable : function_.arguments) {
        auto &var = methodScope.setLocalVariable(variable.variableName, variable.type, true,
                                                 function_.position());
        var.initializeAbsolutely();
    }

    scoper_.postArgumentsHook();
    return methodScope;
}

void FunctionPAG::compileCode(Scope &methodScope) {
    if (hasInstanceScope()) {
        scoper_.instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired());
    }

    if (isFullyInitializedCheckRequired()) {
        auto initializer = static_cast<Initializer &>(function_);
        for (auto &var : initializer.argumentsToVariables()) {
            if (!scoper_.instanceScope()->hasLocalVariable(var)) {
                throw CompilerError(initializer.position(),
                                    "🍼 was applied to \"%s\" but no matching instance variable was found.",
                                    var.utf8().c_str());
            }
            auto &instanceVariable = scoper_.instanceScope()->getLocalVariable(var);
            auto &argumentVariable = methodScope.getLocalVariable(var);
            if (!argumentVariable.type().compatibleTo(instanceVariable.type(), typeContext_)) {
                throw CompilerError(initializer.position(),
                                    "🍼 was applied to \"%s\" but instance variable has incompatible type.",
                                    var.utf8().c_str());
            }
            instanceVariable.initialize(writer_.count());
            produceToVariable(ResolvedVariable(instanceVariable, true));
            box(TypeExpectation(instanceVariable.type()), argumentVariable.type());
            copyVariableContent(ResolvedVariable(argumentVariable, false));
        }
    }

    while (stream_.nextTokenIsEverythingBut(E_WATERMELON)) {
        parseStatement();
        scoper_.clearTemporaryScope();

        if (pathAnalyser.hasCertainly(PathAnalyserIncident::Returned) && !stream_.nextTokenIs(E_WATERMELON)) {
            compilerWarning(stream_.consumeToken().position(), "Dead code.");
            break;
        }
    }

    if (function_.compilationMode() == FunctionPAGMode::ObjectInitializer) {
        writer_.writeInstruction(INS_RETURN);
        box(TypeExpectation(function_.returnType), typeContext_.calleeType());
        writer_.writeInstruction(INS_THIS);
    }
    else if (function_.compilationMode() == FunctionPAGMode::ValueTypeInitializer) {
        writer_.writeInstruction(INS_RETURN_WITHOUT_VALUE);
    }

    if (isFullyInitializedCheckRequired()) {
        auto initializer = static_cast<Initializer &>(function_);
        scoper_.instanceScope()->initializerUnintializedVariablesCheck(initializer.position(),
                                                                       "Instance variable \"%s\" must be initialized.");
    }
    else if (!pathAnalyser.hasCertainly(PathAnalyserIncident::Returned)) {
        if (function_.returnType.type() != TypeContent::Nothingness) {
            throw CompilerError(function_.position(), "An explicit return is missing.");
        }
        else {
            writer_.writeInstruction(INS_RETURN_WITHOUT_VALUE);
        }
    }

    if (isSuperconstructorRequired()) {
        auto initializer = static_cast<Initializer &>(function_);
        if (typeContext_.calleeType().eclass()->superclass() != nullptr &&
            !pathAnalyser.hasCertainly(PathAnalyserIncident::CalledSuperInitializer)) {
            if (pathAnalyser.hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
                throw CompilerError(initializer.position(), "Superinitializer is potentially not called.");
            }
            else {
                throw CompilerError(initializer.position(), "Superinitializer is not called.");
            }
        }
    }
}

void FunctionPAG::compile() {
    try {
        Scope &methodScope = setArgumentVariables();

        if (function_.compilationMode() == FunctionPAGMode::BoxingLayer) {
            generateBoxingLayer(static_cast<BoxingLayer &>(function_));
        }
        else if (function_.isNative()) {
            writer_.writeInstruction(INS_TRANSFER_CONTROL_TO_NATIVE);
        }
        else {
            compileCode(methodScope);
        }

        scoper_.popScope(writer_.count());
    }
    catch (CompilerError &ce) {
        printError(ce);
    }
    function_.setFullSize(scoper_.fullSize());
    function_.setTokenStream(TokenStream());  // Replace the token stream to hopefully release memory
}

void FunctionPAG::generateBoxingLayer(BoxingLayer &layer) {
    if (layer.destinationReturnType().type() != TypeContent::Nothingness) {
        writer_.writeInstruction(INS_RETURN);
        box(TypeExpectation(layer.returnType), layer.destinationReturnType());
    }
    if (layer.owningType().type() == TypeContent::Callable) {
        writer_.writeInstruction({ INS_EXECUTE_CALLABLE, INS_THIS });
    }
    else {
        switch (layer.owningType().type()) {
            case TypeContent::ValueType:
            case TypeContent::Enum:
                writer_.writeInstruction(INS_CALL_CONTEXTED_FUNCTION);
                break;
            case TypeContent::Class:
                writer_.writeInstruction(INS_DISPATCH_METHOD);
                break;
            case TypeContent::Callable:
                break;
            default:
                throw std::logic_error("nonsensial BoxingLayer requested");
        }
        writer_.writeInstruction(INS_THIS);
        writer_.writeInstruction(layer.destinationFunction()->vtiForUse());
    }
    size_t variableIndex = 0;
    for (size_t i = 0; i < layer.destinationArgumentTypes().size(); i++) {
        auto &argType = layer.destinationArgumentTypes()[i];
        writer_.writeInstruction(argType.size());
        box(TypeExpectation(argType), layer.arguments[i].type);
        auto variable = Variable(layer.arguments[i].type, variableIndex, true, EmojicodeString(), layer.position());
        variableIndex += layer.arguments[i].type.size();
        copyVariableContent(ResolvedVariable(variable, false));
    }
    if (layer.destinationReturnType().type() == TypeContent::Nothingness) {
        writer_.writeInstruction(INS_RETURN_WITHOUT_VALUE);
    }
}


FunctionPAG::FunctionPAG(Function &function, Type contextType, FunctionWriter &writer, CallableScoper &scoper)
    : AbstractParser(function.package(), function.tokenStream()), function_(function), writer_(writer), scoper_(scoper),
    typeContext_(typeContextForType(std::move(contextType))) {
        scoper_.setVariableInformation(&function_.objectVariableInformation());
    }

}  // namespace EmojicodeCompiler
