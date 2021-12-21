//
//  ExpressionAnalyser.cpp
//  runtime
//
//  Created by Theo Weidmann on 28.11.18.
//

#include "ExpressionAnalyser.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTVariables.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "Compiler.hpp"
#include "Functions/Function.hpp"
#include "SemanticAnalyser.hpp"
#include "ThunkBuilder.hpp"
#include "Types/Class.hpp"
#include "Types/ValueType.hpp"
#include "Types/CommonTypeFinder.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

ExpressionAnalyser::ExpressionAnalyser(SemanticAnalyser *analyser, TypeContext typeContext, Package *package,
                                       std::unique_ptr<SemanticScoper> scoper)
    : typeContext_(std::move(typeContext)), analyser_(analyser), scoper_(std::move(scoper)), package_(package) {}

ExpressionAnalyser::~ExpressionAnalyser() = default;

void ExpressionAnalyser::error(const CompilerError &ce) const {
    compiler()->error(ce);
}

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

Type ExpressionAnalyser::analyseTypeExpr(const std::shared_ptr<ASTTypeExpr> &node, const TypeExpectation &exp,
                                         bool allowGenericInf) {
    Type type = node->analyse(this, exp, allowGenericInf).resolveOnSuperArgumentsAndConstraints(typeContext_);
    node->setExpressionType(type);
    return type;
}

Type ExpressionAnalyser::expect(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    analyse(*node);
    return comply(expectation, node);
}

Type ExpressionAnalyser::analyse(const std::shared_ptr<ASTExpr> &ptr) {
    assert(ptr->expressionType().is<TypeType::Invalid>() && "Expression was already analysed.");
    Type type = ptr->analyse(this);
    ptr->setExpressionType(type);
    return type;
}

void ExpressionAnalyser::expectType(const Type &type, std::shared_ptr<ASTExpr> *node) {
    analyse(*node);
    auto resultType = comply(TypeExpectation(type), node);
    if (!resultType.compatibleTo(type, typeContext_)) {
        throw CompilerError((*node)->position(), resultType.toString(typeContext_), " is not compatible to ",
                            type.toString(typeContext_), ".");
    }
}

/// Checks that the function can be accessed or issues an error. Checks that the function is not deprecated
/// and issues a warning otherwise.
void checkFunctionSafety(Function *function, const SourcePosition &p, ExpressionAnalyser *analyser) {
    if (function->unsafe() && !analyser->isInUnsafeBlock()) {
        analyser->compiler()->error(CompilerError(p, "Use of unsafe function ", utf8(function->name()), " requires ☣️  block."));
    }
}

/// Issues a warning at the given position if the function is deprecated.
void deprecatedWarning(Function *function, const SourcePosition &p, Compiler *compiler) {
    if (function->deprecated()) {
        if (!function->documentation().empty()) {
            compiler->warn(p, utf8(function->name()), " is deprecated. Please refer to the "\
                             "documentation for further information: ", utf8(function->documentation()));
        }
        else {
            compiler->warn(p, utf8(function->name()), " is deprecated.");
        }
    }
}

Type ExpressionAnalyser::analyseFunctionCall(ASTArguments *node, const Type &type, Function *function) {
    auto genericArgs = transformTypeAstVector(node->genericArguments(), typeContext());

    TypeContext typeContext = TypeContext(type, function, &genericArgs);
    function->requestReificationAndCheck(typeContext, genericArgs, node->position());

    for (size_t i = 0; i < function->parameters().size(); i++) {
        auto &paramType = function->parameters()[i].type->type();
        Type exprType = comply(TypeExpectation(paramType.resolveOn(typeContext)), &node->args()[i]);

        if (function->owner() != compiler()->sMemory) {
            if (paramType.is<TypeType::GenericVariable>()) {  // i.e. the value is not boxed
                insertNode<ASTUpcast>(&node->args()[i], exprType,
                                      type.typeDefinition()->constraintForIndex(paramType.genericVariableIndex()));
            }
            if (paramType.is<TypeType::LocalGenericVariable>()) {  // i.e. the value is not boxed
                insertNode<ASTUpcast>(&node->args()[i], exprType,
                                      function->constraintForIndex(paramType.genericVariableIndex()));
            }
        }
    }
    node->setGenericArgumentTypes(genericArgs);
    auto rtType = function->returnType()->type().resolveOn(typeContext);
    if (rtType.isReference()) {
        rtType.setMutable(type.isMutable());
    }
    checkFunctionSafety(function, node->position(), this);
    deprecatedWarning(function, node->position(), compiler());
    return rtType;
}

Type ExpressionAnalyser::comply(const TypeExpectation &expectation, std::shared_ptr<ASTExpr> *node) {
    assert(!(*node)->expressionType().is<TypeType::Invalid>() &&
            "Expression was not analysed with ExpressionAnalyser::analyse");
    Type exprType = (*node)->comply(this, expectation).resolveOnSuperArgumentsAndConstraints(typeContext());
    (*node)->setExpressionType(exprType);
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
                if (exprType.isReference()) {
                    // This is an edge case caused by ASTInterpolationLiteral.
                    exprType.setReference(false);
                    insertNode<ASTDereference>(node, exprType);
                }
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

bool doStorageTypesMatch(const Type &a, const Type &b, const TypeContext &tc) {
    return a.storageType() == b.storageType() && (a.type() != TypeType::Box || a.areMatchingBoxes(b, tc));
}

bool ExpressionAnalyser::callableBoxingRequired(const TypeExpectation &expectation, const Type &exprType) const {
    if (expectation.type() == TypeType::Callable && exprType.type() == TypeType::Callable &&
        expectation.parametersCount() == exprType.parametersCount()) {
        auto mismatch = std::mismatch(expectation.parameters(), expectation.parametersEnd(),
                                      exprType.parameters(), [this](const Type &a, const Type &b) {
                                          return doStorageTypesMatch(a, b, typeContext());
                                      });
        return mismatch.first != expectation.parametersEnd() ||
            !doStorageTypesMatch(expectation.returnType(), exprType.returnType(), typeContext());
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

}  // namespace EmojicodeCompiler
