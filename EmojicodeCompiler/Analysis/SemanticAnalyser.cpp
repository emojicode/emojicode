//
//  SemanticAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "../AST/ASTNode.hpp"
#include "../AST/ASTBoxing.hpp"
#include "../AST/ASTLiterals.hpp"
#include "../AST/ASTVariables.hpp"
#include "../BoxingLayer.hpp"
#include "../Function.hpp"
#include "../Scoping/VariableNotFoundError.hpp"
#include "../Types/CommonTypeFinder.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeDefinition.hpp"
#include "../Initializer.hpp"
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
                throw CompilerError(initializer->position(),
                                    "🍼 was applied to \"%s\" but no matching instance variable was found.",
                                    var.utf8().c_str());
            }
            auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var);
            auto &argumentVariable = methodScope.getLocalVariable(var);
            if (!argumentVariable.type().compatibleTo(instanceVariable.type(), typeContext_)) {
                throw CompilerError(initializer->position(),
                                    "🍼 was applied to \"%s\" but instance variable has incompatible type.",
                                    var.utf8().c_str());
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

    scoper_->popScope();
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
        if (function_->returnType.type() != TypeType::Nothingness) {
            throw CompilerError(function_->position(), "An explicit return is missing.");
        }
        else {
            root->appendNode(std::make_shared<ASTReturn>(nullptr, root->position()));
        }
    }
}

void SemanticAnalyser::analyseInitializationRequirements() {
    if (isFullyInitializedCheckRequired(function_->functionType())) {
        scoper_->instanceScope()->initializerUnintializedVariablesCheck(function_->position(),
                                                                        "Instance variable \"%s\" must be initialized.");
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
    auto returnType = ctargs ? (*node)->analyse(this, TypeExpectation()) : expect(TypeExpectation(type), node);
    if (!returnType.compatibleTo(type, typeContext_, ctargs)) {
        auto cn = returnType.toString(typeContext_);
        auto tn = type.toString(typeContext_);
        throw CompilerError((*node)->position(), "%s is not compatible to %s.", cn.c_str(), tn.c_str());
    }
    return returnType;
}

void SemanticAnalyser::validateAccessLevel(Function *function, const SourcePosition &p) const {
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

Type SemanticAnalyser::analyseFunctionCall(ASTArguments *node, const Type &type, Function *function) {
    if (node->arguments().size() != function->arguments.size()) {
        throw CompilerError(node->position(), "%s expects %ld arguments but %ld were supplied.",
                            function->name().utf8().c_str(), function->arguments.size(), node->arguments().size());
    }

    TypeContext typeContext = TypeContext(type, function, &node->genericArguments());
    if (node->genericArguments().empty() && !function->genericArgumentVariables.empty()) {
        std::vector<CommonTypeFinder> genericArgsFinders(function->genericArgumentVariables.size(), CommonTypeFinder());

        size_t i = 0;
        for (auto arg : function->arguments) {
            expectType(arg.type.resolveOn(typeContext), &node->arguments()[i++], &genericArgsFinders);
        }
        for (auto &finder : genericArgsFinders) {
            typeContext.functionGenericArguments()->emplace_back(finder.getCommonType(node->position()));
        }
    }
    else if (node->genericArguments().size() != function->genericArgumentVariables.size()) {
        throw CompilerError(node->position(), "Too few generic arguments provided.");
    }

    for (size_t i = 0; i < node->genericArguments().size(); i++) {
        if (!node->genericArguments()[i].compatibleTo(function->genericArgumentConstraints[i], typeContext)) {
            throw CompilerError(node->position(),
                                "Generic argument %d of type %s is not compatible to constraint %s.",
                                i + 1, node->genericArguments()[i].toString(typeContext).c_str(),
                                function->genericArgumentConstraints[i].toString(typeContext).c_str());
        }
    }

    size_t i = 0;
    for (auto arg : function->arguments) {
        expectType(arg.type.resolveOn(typeContext), &node->arguments()[i++]);
    }
    function->deprecatedWarning(node->position());
    validateAccessLevel(function, node->position());
    return function->returnType.resolveOn(typeContext);
}

template<typename T, typename ...Args>
std::shared_ptr<T> insertNode(std::shared_ptr<ASTExpr> *node, const Type &type, Args... args) {
    auto pos = (*node)->position();
    *node = std::make_shared<T>(std::move(*node), type, pos, args...);
    return std::static_pointer_cast<T>(*node);
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

Type SemanticAnalyser::box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    (*node)->setExpressionType(exprType);
    if (exprType.type() == TypeType::ValueType && !exprType.isReference() && expectation.isMutable()) {
        exprType.setMutable(true);
    }
    if (!expectation.shouldPerformBoxing()) {
        return exprType;
    }

    if (callableBoxingRequired(expectation, exprType)) {
        auto arguments = std::vector<Argument>();
        arguments.reserve(expectation.genericArguments().size() - 1);
        for (auto argumentType = expectation.genericArguments().begin() + 1;
             argumentType != expectation.genericArguments().end(); argumentType++) {
            arguments.emplace_back(EmojicodeString(expectation.genericArguments().end() - argumentType),
                                   *argumentType);
        }
        auto destinationArgTypes = std::vector<Type>(exprType.genericArguments().begin() + 1,
                                                     exprType.genericArguments().end());
        auto boxingLayer = new BoxingLayer(destinationArgTypes, exprType.genericArguments()[0],
                                           function_->package(), arguments,
                                           expectation.genericArguments()[0], function_->position());
        buildBoxingLayerAst(boxingLayer);
        function_->package()->registerFunction(boxingLayer);
        Function::analysisQueue.emplace(boxingLayer);

//        insertNode<ASTCallableBox>(node, exprType, boxingLayer);
    }

    if (exprType.isReference() && !expectation.isReference()) {
        exprType.setReference(false);
        insertNode<ASTDereference>(node, exprType);
    }

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
                        auto prty = exprType;
                        exprType = expectation;
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

    if (!exprType.isReference() && expectation.isReference() && exprType.isReferencable()) {
        exprType.setReference(true);
        if (auto varNode = std::dynamic_pointer_cast<ASTGetVariable>(*node)) {
            varNode->setReference();
            varNode->setExpressionType(exprType);
        }
        else {
            auto storeTemp = insertNode<ASTStoreTemporarily>(node, exprType);
            scoper_->pushTemporaryScope();
            auto &var = scoper_->currentScope().declareInternalVariable(exprType, storeTemp->position());
            storeTemp->setVarId(var.id());
        }
    }
    return exprType;
}

Type SemanticAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    return box((*node)->analyse(this, expectation), expectation, node);
}

bool SemanticAnalyser::typeIsEnumerable(const Type &type, Type *elementType) {
    if (type.type() == TypeType::Class && !type.optional()) {
        for (Class *a = type.eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.protocol() == PR_ENUMERATEABLE) {
                    auto itemType = Type(TypeType::GenericVariable, false, 0, PR_ENUMERATEABLE);
                    *elementType = itemType.resolveOn(protocol.resolveOn(type));
                    return true;
                }
            }
        }
    }
    else if (type.canHaveProtocol() && !type.optional()) {
        for (auto &protocol : type.typeDefinition()->protocols()) {
            if (protocol.protocol() == PR_ENUMERATEABLE) {
                auto itemType = Type(TypeType::GenericVariable, false, 0, PR_ENUMERATEABLE);
                *elementType = itemType.resolveOn(protocol.resolveOn(type));
                return true;
            }
        }
    }
    else if (type.type() == TypeType::Protocol && type.protocol() == PR_ENUMERATEABLE) {
        *elementType = Type(TypeType::GenericVariable, false, 0, type.protocol()).resolveOn(type);
        return true;
    }
    return false;
}

}  // namespace EmojicodeCompiler
