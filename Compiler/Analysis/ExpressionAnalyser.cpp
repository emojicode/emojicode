//
//  ExpressionAnalyser.cpp
//  runtime
//
//  Created by Theo Weidmann on 28.11.18.
//

#include "ExpressionAnalyser.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTVariables.hpp"
#include "Compiler.hpp"
#include "Functions/Function.hpp"
#include "SemanticAnalyser.hpp"
#include "ThunkBuilder.hpp"
#include "Types/Class.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

ExpressionAnalyser::ExpressionAnalyser(SemanticAnalyser *analyser, TypeContext typeContext, Package *package,
                                       std::unique_ptr<SemanticScoper> scoper)
    : typeContext_(std::move(typeContext)), analyser_(analyser), scoper_(std::move(scoper)), package_(package) {}

ExpressionAnalyser::~ExpressionAnalyser() = default;

Compiler* ExpressionAnalyser::compiler() const {
    return analyser_->compiler();
}

FunctionType ExpressionAnalyser::functionType() const {
    return FunctionType::Function;
}

Type ExpressionAnalyser::real() const {
    return Type(compiler()->sReal);
}

Type ExpressionAnalyser::integer() const {
    return Type(compiler()->sInteger);
}

Type ExpressionAnalyser::boolean() const {
    return Type(compiler()->sBoolean);
}

Type ExpressionAnalyser::byte() const {
    return Type(compiler()->sByte);
}

void ExpressionAnalyser::configureClosure(Function *closure) const {
    closure->setMutating(false);
    closure->setFunctionType(functionType());
}

Type ExpressionAnalyser::analyseTypeExpr(const std::shared_ptr<ASTExpr> &node, const TypeExpectation &exp) {
    auto type = node->analyse(this, exp).resolveOnSuperArgumentsAndConstraints(typeContext_);
    node->setExpressionType(type);
    return type;
}

Type ExpressionAnalyser::expectType(const Type &type, std::shared_ptr<ASTExpr> *node, std::vector<CommonTypeFinder> *ctargs) {
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

Type ExpressionAnalyser::analyseFunctionCall(ASTArguments *node, const Type &type, Function *function) {
    if (node->args().size() != function->parameters().size()) {
        throw CompilerError(node->position(), utf8(function->name()), " expects ", function->parameters().size(),
                            " arguments but ", node->args().size(), " were supplied.");
    }

    ensureGenericArguments(node, type, function);

    auto genericArgs = transformTypeAstVector(node->genericArguments(), typeContext());

    TypeContext typeContext = TypeContext(type, function, &genericArgs);
    function->requestReificationAndCheck(typeContext, genericArgs, node->position());

    for (size_t i = 0; i < function->parameters().size(); i++) {
        expectType(function->parameters()[i].type->type().resolveOn(typeContext), &node->args()[i]);
    }
    checkFunctionUse(function, node->position());
    node->setGenericArgumentTypes(genericArgs);
    auto rtType = function->returnType()->type().resolveOn(typeContext);
    if (rtType.isReference()) {
        rtType.setMutable(type.isMutable());
    }
    return rtType;
}

void ExpressionAnalyser::checkFunctionUse(Function *function, const SourcePosition &p) const {
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

void ExpressionAnalyser::checkFunctionSafety(Function *function, const SourcePosition &p) const {
    if (function->unsafe() && !isInUnsafeBlock()) {
        compiler()->error(CompilerError(p, "Use of unsafe function ", utf8(function->name()), " requires ☣️  block."));
    }
}

void ExpressionAnalyser::deprecatedWarning(Function *function, const SourcePosition &p) const {
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

void ExpressionAnalyser::ensureGenericArguments(ASTArguments *node, const Type &type, Function *function) {
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

Type ExpressionAnalyser::comply(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    (*node)->setExpressionType(exprType.resolveOnSuperArgumentsAndConstraints(typeContext()));
    if (!expectation.shouldPerformBoxing()) {
        return exprType;
    }

    exprType = upcast(std::move(exprType), expectation, node);
    exprType = callableBox(std::move(exprType), expectation, node);
    exprType = box(std::move(exprType), expectation, node);
    return complyReference(std::move(exprType), expectation, node);
}

Type ExpressionAnalyser::complyReference(Type exprType, const TypeExpectation &expectation,
                                           std::shared_ptr<ASTExpr> *node) const {
    if (!exprType.isReference() && expectation.isReference() && exprType.isReferenceUseful()) {
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

Type ExpressionAnalyser::upcast(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const {
    if ((exprType.type() == TypeType::Class && expectation.type() == TypeType::Class &&
         expectation.klass() != exprType.klass()) || (exprType.unoptionalized().type() == TypeType::Class &&
                                                      expectation.unoptionalized().type() == TypeType::Someobject)) {
        insertNode<ASTUpcast>(node, exprType, expectation.unoptionalized());
        exprType = expectation.unoptionalized();
    }
    return exprType;
}

Type ExpressionAnalyser::box(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const {
    if (exprType.isReference()) {
        if (exprType.storageType() == StorageType::Box && expectation.simplifyType(exprType) == StorageType::Simple) {
            if (exprType.unboxed().isReferenceUseful() && expectation.isReference()) {
                exprType = exprType.unboxed().referenced();
                insertNode<ASTBoxReferenceToReference>(node, exprType);
                return exprType;
            }
            exprType = exprType.unboxed();
            insertNode<ASTBoxReferenceToSimple>(node, exprType);
            return exprType;
        }
        if (!expectation.isReference()) {
            exprType.setReference(false);
            insertNode<ASTDereference>(node, exprType);
        }
    }

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
    }
    return exprType;
}

void ExpressionAnalyser::makeIntoSimpleOptional(Type &exprType, std::shared_ptr<ASTExpr> *node) const {
    switch (exprType.storageType()) {
        case StorageType::SimpleOptional:
        case StorageType::PointerOptional:
            break;
        case StorageType::Box:
            exprType = (*node)->expressionType().unboxed().optionalized();
            insertNode<ASTBoxToSimpleOptional>(node, exprType);
            break;
        case StorageType::Simple:
            exprType = exprType.optionalized();
            insertNode<ASTSimpleToSimpleOptional>(node, exprType);
            break;
    }
}

void ExpressionAnalyser::makeIntoSimple(Type &exprType, std::shared_ptr<ASTExpr> *node) const {
    // Only a box can implicitly be converted to Simple
    if (exprType.storageType() == StorageType::Box) {
        exprType = exprType.unboxed();
        assert(exprType.type() != TypeType::Protocol);
        insertNode<ASTBoxToSimple>(node, exprType);
    }
}

void ExpressionAnalyser::makeIntoBox(Type &exprType, const TypeExpectation &expectation,
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
    }
}

bool ExpressionAnalyser::callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType) const {
    if (expectation.type() == TypeType::Callable && exprType.type() == TypeType::Callable &&
        expectation.genericArguments().size() == exprType.genericArguments().size()) {
        auto mismatch = std::mismatch(expectation.genericArguments().begin(), expectation.genericArguments().end(),
                                      exprType.genericArguments().begin(), [this](const Type &a, const Type &b) {
                                          return a.storageType() == b.storageType() &&
                                            (a.type() != TypeType::Box || a.areMatchingBoxes(b, typeContext()));
                                      });
        return mismatch.first != expectation.genericArguments().end();
    }
    return false;
}

Type ExpressionAnalyser::callableBox(Type exprType, const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) const {
    if (callableBoxingRequired(expectation, exprType)) {
        auto layer = buildCallableThunk(expectation, exprType, package(), (*node)->position());
        analyser_->enqueueFunction(layer.get());
        insertNode<ASTCallableBox>(node, exprType, std::move(layer));
    }
    return exprType;
}

Type ExpressionAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    return comply((*node)->analyse(this, expectation), expectation, node);
}

}  // namespace EmojicodeCompiler
