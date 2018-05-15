//
//  FunctionAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "AST/ASTNode.hpp"
#include "AST/ASTBoxing.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTVariables.hpp"
#include "ThunkBuilder.hpp"
#include "Compiler.hpp"
#include "FunctionAnalyser.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Scoping/VariableNotFoundError.hpp"
#include "SemanticAnalyser.hpp"
#include "Types/Class.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeDefinition.hpp"

namespace EmojicodeCompiler {

Type FunctionAnalyser::real() {
    return Type(compiler()->sReal);
}

Type FunctionAnalyser::integer() {
    return Type(compiler()->sInteger);
}

Type FunctionAnalyser::boolean() {
    return Type(compiler()->sBoolean);
}

Type FunctionAnalyser::symbol() {
    return Type(compiler()->sSymbol);
}

Type FunctionAnalyser::byte() {
    return Type(compiler()->sByte);
}

Type FunctionAnalyser::analyseTypeExpr(const std::shared_ptr<ASTExpr> &node, const TypeExpectation &exp) {
    auto type = node->analyse(this, exp).resolveOnSuperArgumentsAndConstraints(typeContext_);
    node->setExpressionType(type);
    return type;
}

void FunctionAnalyser::checkFunctionUse(Function *function, const SourcePosition &p) const {
    if (function->accessLevel() == AccessLevel::Private) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || function->owningType().typeDefinition() != typeContext_.calleeType().typeDefinition()) {
            compiler()->error(CompilerError(p, utf8(function->name()), " is 🔒."));
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (typeContext_.calleeType().type() != function->owningType().type()
            || !this->typeContext_.calleeType().klass()->inheritsFrom(function->owningType().klass())) {
            compiler()->error(CompilerError(p, utf8(function->name()), " is 🔐."));
        }
    }

    deprecatedWarning(function, p);
    checkFunctionSafety(function, p);
}

void FunctionAnalyser::checkFunctionSafety(Function *function, const SourcePosition &p) const {
    if (function->unsafe() && !inUnsafeBlock_) {
        compiler()->error(CompilerError(p, "Use of unsafe function ", utf8(function->name()), " requires ☣️  block."));
    }
}

void FunctionAnalyser::deprecatedWarning(Function *function, const SourcePosition &p) const {
    if (function->deprecated()) {
        if (!function->documentation().empty()) {
            compiler()->warn(p, utf8(function->name()), " is deprecated. Please refer to the "\
            "documentation for further information: ", utf8(function->documentation()));
        }
        else {
            compiler()->warn(p, utf8(function->name()), " is deprecated.");
        }
    }
}

void FunctionAnalyser::analyse() {
    Scope &methodScope = scoper_->pushArgumentsScope(function_->parameters(), function_->position());

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
            auto assign = std::make_shared<ASTInstanceVariableInitialization>(instanceVariable.name(),
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

void FunctionAnalyser::analyseReturn(const std::shared_ptr<ASTBlock> &root) {
    if (function_ == function_->package()->startFlagFunction() && function_->returnType() == Type::noReturn()) {
        function_->setReturnType(integer());
        auto value = std::make_shared<ASTNumberLiteral>(static_cast<int64_t>(0), std::u32string(), root->position());
        root->appendNode(std::make_shared<ASTReturn>(value, root->position()));
    }
    else if (function_->functionType() == FunctionType::ObjectInitializer &&
            !pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        auto initializer = dynamic_cast<Initializer *>(function_);
        auto thisNode = std::dynamic_pointer_cast<ASTExpr>(std::make_shared<ASTThis>(root->position()));
        comply(typeContext().calleeType(), TypeExpectation(initializer->constructedType(typeContext().calleeType())),
               &thisNode);
        root->appendNode(std::make_shared<ASTReturn>(thisNode, root->position()));
    }
    else if (function_->functionType() == FunctionType::ValueTypeInitializer) {
        root->appendNode(std::make_shared<ASTReturn>(nullptr, root->position()));
    }
    else if (!pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        if (function_->returnType().type() != TypeType::NoReturn) {
            compiler()->error(CompilerError(function_->position(), "An explicit return is missing."));
        }
        else {
            root->appendNode(std::make_shared<ASTReturn>(nullptr, root->position()));
        }
    }
}

void FunctionAnalyser::analyseInitializationRequirements() {
    if (isFullyInitializedCheckRequired(function_->functionType())) {
        scoper_->instanceScope()->uninitializedVariablesCheck(function_->position(), "Instance variable \"",
                                                              "\" must be initialized.");
    }

    if (isSuperconstructorRequired(function_->functionType())) {
        if (typeContext_.calleeType().klass()->superclass() != nullptr &&
            !pathAnalyser_.hasCertainly(PathAnalyserIncident::CalledSuperInitializer)) {
            if (pathAnalyser_.hasPotentially(PathAnalyserIncident::CalledSuperInitializer)) {
                compiler()->error(CompilerError(function_->position(), "Superinitializer is potentially not called."));
            }
            else {
                compiler()->error(CompilerError(function_->position(), "Superinitializer is not called."));
            }
        }
    }
}

Type FunctionAnalyser::expectType(const Type &type, std::shared_ptr<ASTExpr> *node, std::vector<CommonTypeFinder> *ctargs) {
    auto returnType = (*node)->analyse(this, ctargs != nullptr ? TypeExpectation() : TypeExpectation(type));
    if (!returnType.compatibleTo(type, typeContext_, ctargs)) {
        throw CompilerError((*node)->position(), returnType.toString(typeContext_), " is not compatible to ",
                            type.toString(typeContext_), ".");
    }
    if (ctargs == nullptr) {
        comply(returnType, TypeExpectation(type), node);
    }
    return returnType;
}

Type FunctionAnalyser::analyseFunctionCall(ASTArguments *node, const Type &type, Function *function) {
    if (node->parameters().size() != function->parameters().size()) {
        throw CompilerError(node->position(), utf8(function->name()), " expects ", function->parameters().size(),
                            " arguments but ", node->parameters().size(), " were supplied.");
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

    for (size_t i = 0; i < function->parameters().size(); i++) {
        expectType(function->parameters()[i].type.resolveOn(typeContext), &node->parameters()[i]);
    }
    function->requestReification(node->genericArguments());
    checkFunctionUse(function, node->position());
    return function->returnType().resolveOn(typeContext);
}

void FunctionAnalyser::ensureGenericArguments(ASTArguments *node, const Type &type, Function *function) {
    if (node->genericArguments().empty() && !function->genericParameters().empty()) {
        std::vector<CommonTypeFinder> genericArgsFinders(function->genericParameters().size(), CommonTypeFinder());
        TypeContext typeContext = TypeContext(type, function, nullptr);
        size_t i = 0;
        for (auto arg : function->parameters()) {
            expectType(arg.type.resolveOn(typeContext), &node->parameters()[i++], &genericArgsFinders);
        }
        for (auto &finder : genericArgsFinders) {
            node->genericArguments().emplace_back(finder.getCommonType(node->position(), compiler()));
        }
    }
    else if (node->genericArguments().size() != function->genericParameters().size()) {
        throw CompilerError(node->position(), "Too few generic arguments provided.");
    }
}

template<typename T, typename ...Args>
std::shared_ptr<T> insertNode(std::shared_ptr<ASTExpr> *node, const Type &type, Args... args) {
    auto pos = (*node)->position();
    *node = std::make_shared<T>(std::move(*node), type, pos, args...);
    return std::static_pointer_cast<T>(*node);
}

Type FunctionAnalyser::comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    (*node)->setExpressionType(exprType.resolveOnSuperArgumentsAndConstraints(typeContext()));
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

Type FunctionAnalyser::box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    switch (expectation.simplifyType(exprType)) {
        case StorageType::SimpleOptional:
            makeIntoSimpleOptional(exprType, node);
            break;
        case StorageType::Box:
            makeIntoBox(exprType, expectation, node);
            break;
        case StorageType::Simple:
            makeIntoSimple(exprType, node);
            break;
        case StorageType::SimpleError:
            makeIntoSimpleError(exprType, node, expectation);
            break;
    }
    return exprType;
}

void FunctionAnalyser::makeIntoSimpleOptional(Type &exprType, std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::SimpleOptional:
        case StorageType::SimpleError:
            break;
        case StorageType::Box:
            exprType = Type(MakeOptional, exprType.unboxed());
            insertNode<ASTBoxToSimpleOptional>(node, exprType);
            break;
        case StorageType::Simple:
            exprType = Type(MakeOptional, exprType);
            insertNode<ASTSimpleToSimpleOptional>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoSimpleError(Type &exprType, std::shared_ptr<ASTExpr> *node, const TypeExpectation &exp) {
    switch (exprType.storageType()) {
        case StorageType::SimpleError:
        case StorageType::SimpleOptional:
            break;
        case StorageType::Simple:
            exprType = Type(MakeError, exp.errorEnum(), exprType.unboxed());
            insertNode<ASTSimpleToSimpleError>(node, exprType);
            break;
        case StorageType::Box:
            exprType = Type(MakeError, exp.errorEnum(), exprType.unboxed());
            insertNode<ASTBoxToSimpleError>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoSimple(Type &exprType, std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::Simple:
        case StorageType::SimpleOptional:
        case StorageType::SimpleError:
            break;
        case StorageType::Box:
            exprType.unbox();
            insertNode<ASTBoxToSimple>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoBox(Type &exprType, const TypeExpectation &expectation,
                                   std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::Box:
            break;
        case StorageType::SimpleOptional:
            exprType.forceBox();
            insertNode<ASTSimpleOptionalToBox>(node, exprType, expectation.copyType());
            break;
        case StorageType::Simple:
            exprType.forceBox();
            insertNode<ASTSimpleToBox>(node, exprType, expectation.copyType());
            break;
        case StorageType::SimpleError:
            exprType.forceBox();
            insertNode<ASTSimpleErrorToBox>(node, exprType, expectation.copyType());
            break;
    }
}

bool FunctionAnalyser::callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType) {
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

Type FunctionAnalyser::callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    if (callableBoxingRequired(expectation, exprType)) {
        auto layer = buildBoxingThunk(expectation, exprType, function()->package(), (*node)->position());
        insertNode<ASTCallableBox>(node, exprType, layer.get());
        analyser_->enqueueFunction(layer.get());
        function()->package()->add(std::move(layer));
    }
    return exprType;
}

Type FunctionAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    return comply((*node)->analyse(this, expectation), expectation, node);
}

}  // namespace EmojicodeCompiler
