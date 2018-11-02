//
//  FunctionAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "AST/ASTBoxing.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTVariables.hpp"
#include "Compiler.hpp"
#include "FunctionAnalyser.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "SemanticAnalyser.hpp"
#include "ThunkBuilder.hpp"
#include "Package/Package.hpp"
#include "Types/Class.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

FunctionAnalyser::FunctionAnalyser(Function *function, SemanticAnalyser *analyser) :
    scoper_(std::make_unique<SemanticScoper>(SemanticScoper::scoperForFunction(function))),
    typeContext_(function->typeContext()), function_(function), analyser_(analyser),
    inUnsafeBlock_(function->unsafe()) {}

FunctionAnalyser::FunctionAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper,
                                   SemanticAnalyser *analyser) :
    scoper_(std::move(scoper)), typeContext_(function->typeContext()), function_(function),
    analyser_(analyser), inUnsafeBlock_(function->unsafe()) {}

FunctionAnalyser::~FunctionAnalyser() = default;

Compiler* FunctionAnalyser::compiler() const {
    return function_->package()->compiler();
}

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
    auto callee = typeContext_.calleeType();
    if (callee.type() == TypeType::TypeAsValue) {
        callee = callee.typeOfTypeValue();
    }
    if (function->accessLevel() == AccessLevel::Private) {
        if (callee.type() != function->owner()->type().type() || function->owner() != callee.typeDefinition()) {
            compiler()->error(CompilerError(p, utf8(function->name()), " is 🔒."));
        }
    }
    else if (function->accessLevel() == AccessLevel::Protected) {
        if (callee.type() != function->owner()->type().type()
            || !callee.klass()->inheritsFrom(dynamic_cast<Class *>(function->owner()))) {
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

    if (function_->functionType() == FunctionType::ObjectInitializer &&
        dynamic_cast<Class *>(function_->owner())->foreign()) {
        compiler()->error(CompilerError(function_->position(), "Foreign classes cannot have native initializers."));
    }

    if (hasInstanceScope(function_->functionType())) {
        scoper_->instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired(function_->functionType()));
    }

    if (isFullyInitializedCheckRequired(function_->functionType())) {
        auto initializer = dynamic_cast<Initializer *>(function_);
        for (auto &var : initializer->owner()->instanceVariables()) {
            if (var.type->type().type() == TypeType::Optional) {
                auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var.name);
                auto noValue = std::make_shared<ASTNoValue>(initializer->position());
                auto assign = std::make_unique<ASTInstanceVariableInitialization>(instanceVariable.name(),
                                                                                  noValue, initializer->position());
                function()->ast()->appendNode(std::move(assign));
            }
        }
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
            auto assign = std::make_unique<ASTInstanceVariableInitialization>(instanceVariable.name(),
                                                                              getVar, initializer->position());
            function()->ast()->appendNode(std::move(assign));
        }
    }

    function_->ast()->analyse(this);

    analyseReturn(function()->ast());
    analyseInitializationRequirements();

    function_->ast()->setScopeStats(scoper_->popScope(compiler()));
    function_->setVariableCount(scoper_->variableIdCount());
}

void FunctionAnalyser::analyseReturn(ASTBlock *root) {
    if (function_ == function_->package()->startFlagFunction() && function_->returnType()->type() == Type::noReturn()) {
        function_->setReturnType(std::make_unique<ASTLiteralType>(integer()));
        auto value = std::make_shared<ASTNumberLiteral>(static_cast<int64_t>(0), std::u32string(), root->position());
        root->appendNode(std::make_unique<ASTReturn>(value, root->position()));
    }
    else if (function_->functionType() == FunctionType::ObjectInitializer &&
            !pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        auto initializer = dynamic_cast<Initializer *>(function_);
        auto thisNode = std::dynamic_pointer_cast<ASTExpr>(std::make_shared<ASTThis>(root->position()));
        comply(typeContext().calleeType(), TypeExpectation(initializer->constructedType(typeContext().calleeType())),
               &thisNode);
        auto ret = std::make_unique<ASTReturn>(thisNode, root->position());
        ret->setIsInitReturn();
        root->appendNode(std::move(ret));
    }
    else if (function_->functionType() == FunctionType::ValueTypeInitializer) {
        root->appendNode(std::make_unique<ASTReturn>(nullptr, root->position()));
    }
    else if (!pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        if (function_->returnType()->type().type() != TypeType::NoReturn) {
            compiler()->error(CompilerError(function_->position(), "An explicit return is missing."));
        }
        else {
            root->appendNode(std::make_unique<ASTReturn>(nullptr, root->position()));
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
    if (node->args().size() != function->parameters().size()) {
        throw CompilerError(node->position(), utf8(function->name()), " expects ", function->parameters().size(),
                            " arguments but ", node->args().size(), " were supplied.");
    }

    ensureGenericArguments(node, type, function);

    auto genericArgs = transformTypeAstVector(node->genericArguments(), typeContext());

    TypeContext typeContext = TypeContext(type, function, &genericArgs);
    function->checkGenericArguments(typeContext, genericArgs, node->position());

    for (size_t i = 0; i < function->parameters().size(); i++) {
        expectType(function->parameters()[i].type->type().resolveOn(typeContext), &node->args()[i]);
    }
    function->requestReification(genericArgs);
    checkFunctionUse(function, node->position());
    node->setGenericArgumentTypes(genericArgs);
    return function->returnType()->type().resolveOn(typeContext);
}

void FunctionAnalyser::ensureGenericArguments(ASTArguments *node, const Type &type, Function *function) {
    if (node->genericArguments().empty() && !function->genericParameters().empty()) {
        std::vector<CommonTypeFinder> genericArgsFinders(function->genericParameters().size(), CommonTypeFinder());
        TypeContext typeContext = TypeContext(type, function, nullptr);
        size_t i = 0;
        for (auto &arg : function->parameters()) {
            expectType(arg.type->type().resolveOn(typeContext), &node->args()[i++], &genericArgsFinders);
        }
        for (auto &finder : genericArgsFinders) {
            auto commonType = finder.getCommonType(node->position(), compiler());
            node->genericArguments().emplace_back(std::make_unique<ASTLiteralType>(commonType));
        }
    }
}

Type FunctionAnalyser::comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    (*node)->setExpressionType(exprType.resolveOnSuperArgumentsAndConstraints(typeContext()));
    if (exprType.type() == TypeType::ValueType && !exprType.isReference() && expectation.isMutable()) {
        exprType.setMutable(true);
    }
    if (!expectation.shouldPerformBoxing()) {
        return exprType;
    }

    upcast(exprType, expectation, node);

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

void FunctionAnalyser::upcast(Type &exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    if ((exprType.type() == TypeType::Class && expectation.type() == TypeType::Class &&
         expectation.klass() != exprType.klass()) || (exprType.unoptionalized().type() == TypeType::Class &&
                                                      expectation.unoptionalized().type() == TypeType::Someobject)) {
        insertNode<ASTUpcast>(node, exprType, expectation.unoptionalized());
        exprType = expectation.unoptionalized();
    }
}

Type FunctionAnalyser::box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    switch (expectation.simplifyType(exprType)) {
        case StorageType::SimpleOptional:
        case StorageType::PointerOptional:
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
            case StorageType::PointerOptional:
        case StorageType::SimpleError:
            break;
        case StorageType::Box:
            exprType = exprType.unboxed().optionalized();
            insertNode<ASTBoxToSimpleOptional>(node, exprType);
            break;
        case StorageType::Simple:
            exprType = exprType.optionalized();
            insertNode<ASTSimpleToSimpleOptional>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoSimpleError(Type &exprType, std::shared_ptr<ASTExpr> *node, const TypeExpectation &exp) {
    switch (exprType.storageType()) {
        case StorageType::SimpleError:
        case StorageType::SimpleOptional:
        case StorageType::PointerOptional:
            break;
        case StorageType::Simple:
            exprType = exprType.unboxed().errored(exp.errorEnum());
            insertNode<ASTSimpleToSimpleError>(node, exprType);
            break;
        case StorageType::Box:
            exprType = exprType.unboxed().errored(exp.errorEnum());
            insertNode<ASTBoxToSimpleError>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoSimple(Type &exprType, std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::Simple:
        case StorageType::SimpleOptional:
        case StorageType::SimpleError:
        case StorageType::PointerOptional:
            break;
        case StorageType::Box:
            exprType = exprType.unboxed();
            assert(exprType.type() != TypeType::Protocol);
            insertNode<ASTBoxToSimple>(node, exprType);
            break;
    }
}

void FunctionAnalyser::makeIntoBox(Type &exprType, const TypeExpectation &expectation,
                                   std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::Box:
            if (expectation.type() == TypeType::Box &&
                !exprType.boxedFor().identicalTo(expectation.boxedFor(), typeContext(), nullptr)) {
                exprType = exprType.unboxed().boxedFor(expectation.boxedFor());
                insertNode<ASTRebox>(node, exprType);
            }
            break;
        case StorageType::SimpleOptional:
        case StorageType::PointerOptional:
            exprType = exprType.boxedFor(expectation.boxFor());
            insertNode<ASTSimpleOptionalToBox>(node, exprType);
            break;
        case StorageType::Simple:
            exprType = exprType.boxedFor(expectation.boxFor());
            insertNode<ASTSimpleToBox>(node, exprType);
            break;
        case StorageType::SimpleError:
            exprType = exprType.boxedFor(expectation.boxFor());
            insertNode<ASTSimpleErrorToBox>(node, exprType);
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
        auto layer = buildCallableThunk(expectation, exprType, function()->package(), (*node)->position());
        analyser_->enqueueFunction(layer.get());
        insertNode<ASTCallableBox>(node, exprType, std::move(layer));
    }
    return exprType;
}

Type FunctionAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    return comply((*node)->analyse(this, expectation), expectation, node);
}

}  // namespace EmojicodeCompiler
