//
//  SemanticAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "AST/ASTNode.hpp"
#include "AST/ASTBoxing.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTVariables.hpp"
#include "AST/ASTInitialization.hpp"
#include "Compiler.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Scoping/VariableNotFoundError.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/Class.hpp"
#include "Types/TypeDefinition.hpp"
#include "BoxingLayerBuilder.hpp"
#include "SemanticAnalyser.hpp"
#include <memory>

namespace EmojicodeCompiler {

void SemanticAnalyser::analyse() {
    Scope &methodScope = scoper_->pushArgumentsScope(function_->arguments, function_->position());

    if (hasInstanceScope(function_->functionType())) {
        scoper_->instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired(function_->functionType()));
    }

    if (isFullyInitializedCheckRequired(function_->functionType())) {
        auto initializer = dynamic_cast<Initializer *>(function_);
        for (auto &var : initializer->argumentsToVariables()) {
            if (!scoper_->instanceScope()->hasLocalVariable(var)) {
                throw CompilerError(initializer->position(), "🍼 was applied to \"", utf8(var),
                                    "\" but no matching instance variable was found.");
            }
            auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var);
            auto &argumentVariable = methodScope.getLocalVariable(var);
            if (!argumentVariable.type().compatibleTo(instanceVariable.type(), typeContext_)) {
                throw CompilerError(initializer->position(), "🍼 was applied to \"",
                                    utf8(var), "\" but instance variable has incompatible type.");
            }
            instanceVariable.initialize();

            auto getVar = std::make_shared<ASTGetVariable>(argumentVariable.name(), initializer->position());
            auto assign = std::make_shared<ASTInstanceVariableAssignment>(instanceVariable.name(),
                                                                          getVar, initializer->position());
            function()->ast()->preprendNode(assign);
        }
    }

    function_->ast()->analyse(this);

    analyseReturn(function()->ast());
    analyseInitializationRequirements();

    scoper_->popScope(compiler());
    function_->setVariableCount(scoper_->variableIdCount());
}

void SemanticAnalyser::analyseReturn(const std::shared_ptr<ASTBlock> &root) {
    if (function_->functionType() == FunctionType::ObjectInitializer) {
        auto thisNode = std::make_shared<ASTThis>(root->position());
        root->appendNode(std::make_shared<ASTReturn>(thisNode, root->position()));
    }
    else if (function_->functionType() == FunctionType::ValueTypeInitializer) {
        root->appendNode(std::make_shared<ASTReturn>(nullptr, root->position()));
    }
    else if (!pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        if (function_->returnType.type() != TypeType::NoReturn) {
            throw CompilerError(function_->position(), "An explicit return is missing.");
        }
        else {
            root->appendNode(std::make_shared<ASTReturn>(nullptr, root->position()));
        }
    }
}

void SemanticAnalyser::analyseInitializationRequirements() {
    if (isFullyInitializedCheckRequired(function_->functionType())) {
        scoper_->instanceScope()->unintializedVariablesCheck(function_->position(), "Instance variable \"",
                                                             "\" must be initialized.");
    }

    if (isSuperconstructorRequired(function_->functionType())) {
        if (typeContext_.calleeType().eclass()->superclass() != nullptr &&
            !pathAnalyser_.hasCertainly(PathAnalyserIncident::CalledSuperInitializer)) {
            if (pathAnalyser_.hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
                throw CompilerError(function_->position(), "Superinitializer is potentially not called.");
            }
            else {
                throw CompilerError(function_->position(), "Superinitializer is not called.");
            }
        }
    }
}

Type SemanticAnalyser::expectType(const Type &type, std::shared_ptr<ASTExpr> *node, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = ctargs != nullptr ? (*node)->analyse(this, TypeExpectation()) : expect(TypeExpectation(type),
                                                                                             node);
    if (!returnType.compatibleTo(type, typeContext_, ctargs)) {
        throw CompilerError((*node)->position(), returnType.toString(typeContext_), " is not compatible to ",
                            type.toString(typeContext_), ".");
    }
    return returnType;
}

void SemanticAnalyser::validateAccessLevel(Function *function, const SourcePosition &p) const {
    if (function->accessLevel() == AccessLevel::Private) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || function->owningType().typeDefinition() != typeContext_.calleeType().typeDefinition()) {
            throw CompilerError(p, utf8(function->name()), " is 🔒.");
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || !this->typeContext_.calleeType().eclass()->inheritsFrom(function->owningType().eclass())) {
            throw CompilerError(p, utf8(function->name()), " is 🔐.");
        }
    }
}

Type SemanticAnalyser::analyseFunctionCall(ASTArguments *node, const Type &type, Function *function) {
    if (node->arguments().size() != function->arguments.size()) {
        throw CompilerError(node->position(), utf8(function->name()), " expects ", function->arguments.size(),
                            " arguments but ", node->arguments().size(), " were supplied.");
    }

    ensureGenericArguments(node, type, function);

    TypeContext typeContext = TypeContext(type, function, &node->genericArguments());
    for (size_t i = 0; i < node->genericArguments().size(); i++) {
        if (!node->genericArguments()[i].compatibleTo(function->constraintForIndex(i), typeContext)) {
            throw CompilerError(node->position(), "Generic argument ", i + 1, " of type ",
                                node->genericArguments()[i].toString(typeContext), " is not compatible to constraint ",
                                function->constraintForIndex(i).toString(typeContext), ".");
        }
    }

    for (size_t i = 0; i < function->arguments.size(); i++) {
        expectType(function->arguments[i].type.resolveOn(typeContext), &node->arguments()[i]);
    }
    function->deprecatedWarning(node->position());
    validateAccessLevel(function, node->position());
    return function->returnType.resolveOn(typeContext);
}

void SemanticAnalyser::ensureGenericArguments(ASTArguments *node, const Type &type, Function *function) {
    if (node->genericArguments().empty() && function->genericParameterCount() > 0) {
        std::vector<CommonTypeFinder> genericArgsFinders(function->genericParameterCount(), CommonTypeFinder());
        TypeContext typeContext = TypeContext(type, function, nullptr);
        size_t i = 0;
        for (auto arg : function->arguments) {
            expectType(arg.type.resolveOn(typeContext), &node->arguments()[i++], &genericArgsFinders);
        }
        for (auto &finder : genericArgsFinders) {
            node->genericArguments().emplace_back(finder.getCommonType(node->position(), compiler()));
        }
    }
    else if (node->genericArguments().size() != function->genericParameterCount()) {
        throw CompilerError(node->position(), "Too few generic arguments provided.");
    }
}

template<typename T, typename ...Args>
std::shared_ptr<T> insertNode(std::shared_ptr<ASTExpr> *node, const Type &type, Args... args) {
    auto pos = (*node)->position();
    *node = std::make_shared<T>(std::move(*node), type, pos, args...);
    return std::static_pointer_cast<T>(*node);
}

Type SemanticAnalyser::comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    (*node)->setExpressionType(exprType);
    if (exprType.type() == TypeType::ValueType && !exprType.isReference() && expectation.isMutable()) {
        exprType.setMutable(true);
    }
    if (!expectation.shouldPerformBoxing()) {
        return exprType;
    }

    exprType = callableBox(std::move(exprType), expectation, node);

    if (exprType.isReference() && !expectation.isReference()) {
        exprType.setReference(false);
        insertNode<ASTDereference>(node, exprType);
    }

    exprType = box(std::move(exprType), expectation, node);

    if (!exprType.isReference() && expectation.isReference() && exprType.isReferencable()) {
        exprType.setReference(true);
        if (auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(*node)) {
            varNode->setReference();
            varNode->setExpressionType(exprType);
        }
        else {
            insertNode<ASTStoreTemporarily>(node, exprType);
        }
    }
    return exprType;
}

Type SemanticAnalyser::box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    switch (expectation.simplifyType(exprType)) {
        case StorageType::SimpleOptional:
            switch (exprType.storageType()) {
                case StorageType::SimpleOptional:
                    break;
                case StorageType::Box:
                    exprType.unbox();
                    exprType.setOptional();  // TODO: ERROR?!

                    insertNode<ASTBoxToSimpleOptional>(node, exprType);
                    break;
                case StorageType::Simple:
                    if (expectation.type() != TypeType::Error) {
                        exprType.setOptional();
                    }
                    else {
                        Type prty = exprType;
                        exprType = expectation.copyType();
                        exprType.setGenericArgument(1, prty);
                    }
                    insertNode<ASTSimpleToSimpleOptional>(node, exprType);
                    break;
            }
            break;
        case StorageType::Box:
            switch (exprType.storageType()) {
                case StorageType::Box:
                    break;
                case StorageType::SimpleOptional:
                    exprType.forceBox();
                    insertNode<ASTSimpleOptionalToBox>(node, exprType);
                    break;
                case StorageType::Simple:
                    exprType.forceBox();
                    insertNode<ASTSimpleToBox>(node, exprType);
                    break;
            }
            break;
        case StorageType::Simple:
            switch (exprType.storageType()) {
                case StorageType::Simple:
                case StorageType::SimpleOptional:
                    break;
                case StorageType::Box:
                    exprType.unbox();
                    insertNode<ASTBoxToSimple>(node, exprType);
                    break;
            }
            break;
    }
    return exprType;
}

bool SemanticAnalyser::callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType) {
    if (expectation.type() == TypeType::Callable && exprType.type() == TypeType::Callable &&
        expectation.genericArguments().size() == exprType.genericArguments().size()) {
        auto mismatch = std::mismatch(expectation.genericArguments().begin(), expectation.genericArguments().end(),
                                      exprType.genericArguments().begin(), [](const Type &a, const Type &b) {
                                          return a.storageType() == b.storageType();
                                      });
        return mismatch.first != expectation.genericArguments().end();
    }
    return false;
}

Type SemanticAnalyser::callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    if (callableBoxingRequired(expectation, exprType)) {
        auto arguments = std::vector<Argument>();
        arguments.reserve(expectation.genericArguments().size() - 1);
        for (auto argumentType = expectation.genericArguments().begin() + 1;
             argumentType != expectation.genericArguments().end(); argumentType++) {
            arguments.emplace_back(std::u32string(1, expectation.genericArguments().end() - argumentType),
                                   *argumentType);
        }
        auto boxingLayer = std::make_unique<BoxingLayer>(exprType, function_->package(), arguments,
                                                         expectation.genericArguments()[0], function_->position());
        buildBoxingLayerAst(boxingLayer.get());
        insertNode<ASTCallableBox>(node, exprType, boxingLayer.get());
        compiler()->analysisQueue.emplace(boxingLayer.get());
        function_->package()->add(std::move(boxingLayer));
    }
    return exprType;
}

Type SemanticAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    return comply((*node)->analyse(this, expectation), expectation, node);
}

bool SemanticAnalyser::typeIsEnumerable(const Type &type, Type *elementType) {
    if (type.type() == TypeType::Class && !type.optional()) {
        for (Class *a = type.eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.protocol() == PR_ENUMERATEABLE) {
                    auto itemType = Type(false, 0, PR_ENUMERATEABLE);
                    *elementType = itemType.resolveOn(TypeContext(protocol.resolveOn(TypeContext(type))));
                    return true;
                }
            }
        }
    }
    else if (type.canHaveProtocol() && !type.optional()) {
        for (auto &protocol : type.typeDefinition()->protocols()) {
            if (protocol.protocol() == PR_ENUMERATEABLE) {
                auto itemType = Type(   false, 0, PR_ENUMERATEABLE);
                *elementType = itemType.resolveOn(TypeContext(protocol.resolveOn(TypeContext(type))));
                return true;
            }
        }
    }
    else if (type.type() == TypeType::Protocol && type.protocol() == PR_ENUMERATEABLE) {
        *elementType = Type(false, 0, type.protocol()).resolveOn(TypeContext(type));
        return true;
    }
    return false;
}

}  // namespace EmojicodeCompiler
