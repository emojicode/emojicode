//
//  FunctionAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "FunctionAnalyser.hpp"
#include "AST/ASTBoxing.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTVariables.hpp"
#include "Compiler.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "SemanticAnalyser.hpp"
#include "Types/Class.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/TypeExpectation.hpp"

namespace EmojicodeCompiler {

FunctionAnalyser::FunctionAnalyser(Function *function, SemanticAnalyser *analyser) :
    ExpressionAnalyser(analyser, function->typeContext(), function->package(),
                       std::make_unique<SemanticScoper>(SemanticScoper::scoperForFunction(function))),
    function_(function), inUnsafeBlock_(function->unsafe()) {}

FunctionAnalyser::FunctionAnalyser(Function *function, std::unique_ptr<SemanticScoper> scoper,
                                   SemanticAnalyser *analyser) :
    ExpressionAnalyser(analyser, function->typeContext(), function->package(), std::move(scoper)),
    function_(function), inUnsafeBlock_(function->unsafe()) {}

FunctionAnalyser::~FunctionAnalyser() = default;

FunctionType FunctionAnalyser::functionType() const {
    return function()->functionType();
}

void FunctionAnalyser::configureClosure(Function *closure) const {
    closure->setMutating(function()->mutating());
    closure->setOwner(function()->owner());
    auto functionType = function()->functionType();
    if (functionType == FunctionType::ObjectInitializer) {
        closure->setFunctionType(FunctionType::ObjectMethod);
    }
    else if (functionType == FunctionType::ValueTypeInitializer) {
        closure->setFunctionType(FunctionType::ValueTypeMethod);
    }
    else {
        closure->setFunctionType(function()->functionType());
    }
}

void FunctionAnalyser::checkThisUse(const SourcePosition &p) const {
    if (isSuperconstructorRequired(function()->functionType()) &&
        !pathAnalyser_.hasCertainly(PathAnalyserIncident::CalledSuperInitializer) &&
        typeContext().calleeType().klass()->superclass() != nullptr) {
        compiler()->error(CompilerError(p, "Attempt to use 🐕 before superinitializer call."));
    }
    if (isFullyInitializedCheckRequired(function()->functionType())) {
        scoper_->instanceScope()->uninitializedVariablesCheck(p, "Instance variable \"",
                                                              "\" must be initialized before using 🐕.");
    }
}

void FunctionAnalyser::analyse() {
   scoper_->pushArgumentsScope(function_->parameters(), function_->position());

    if (function_->functionType() == FunctionType::ObjectInitializer &&
        dynamic_cast<Class *>(function_->owner())->foreign()) {
        compiler()->error(CompilerError(function_->position(), "Foreign classes cannot have native initializers."));
    }

    if (hasInstanceScope(function_->functionType())) {
        scoper_->instanceScope()->setVariableInitialization(!isFullyInitializedCheckRequired(function_->functionType()));
    }

    if (isFullyInitializedCheckRequired(function_->functionType())) {
        analyseBabyBottle();
        // This comes after baby bottle initialization because we *prepend* nodes, which means they will end up
        // before the baby bottle initialization code.
        initOptionalInstanceVariables();
    }

    function_->ast()->analyse(this);

    analyseReturn(function()->ast());
    analyseInitializationRequirements();

    function_->ast()->popScope(this);
    function_->setVariableCount(scoper_->variableIdCount());
}

void FunctionAnalyser::analyseBabyBottle() {
    auto initializer = dynamic_cast<Initializer *>(function_);
    for (auto &var : initializer->argumentsToVariables()) {
        if (!scoper_->instanceScope()->hasLocalVariable(var)) {
            throw CompilerError(initializer->position(), "🍼 was applied to \"", utf8(var),
                                "\" but no matching instance variable was found.");
        }
        auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var);
        auto &argumentVariable = scoper_->currentScope().getLocalVariable(var);
        if (!argumentVariable.type().compatibleTo(instanceVariable.type(), typeContext_)) {
            throw CompilerError(initializer->position(), "🍼 was applied to \"",
                                utf8(var), "\" but instance variable has incompatible type.");
        }
        instanceVariable.initialize();

        auto getVar = std::make_shared<ASTGetVariable>(argumentVariable.name(), initializer->position());
        auto assign = std::make_unique<ASTInstanceVariableInitialization>(instanceVariable.name(),
                                                                          getVar, initializer->position(), true);
        function()->ast()->prependNode(std::move(assign));
    }
}

void FunctionAnalyser::initOptionalInstanceVariables() {
    for (auto &var : function()->owner()->instanceVariables()) {
        if (var.expr != nullptr) {
            auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var.name);
            auto assign = std::make_unique<ASTInstanceVariableInitialization>(instanceVariable.name(),
                                                                              var.expr, function()->position(), false);
            function()->ast()->prependNode(std::move(assign));
        }
        else if (var.type->type().type() == TypeType::Optional) {
            auto &instanceVariable = scoper_->instanceScope()->getLocalVariable(var.name);
            auto noValue = std::make_shared<ASTNoValue>(function()->position());
            auto assign = std::make_unique<ASTInstanceVariableInitialization>(instanceVariable.name(),
                                                                              noValue, function()->position(), true);
            function()->ast()->prependNode(std::move(assign));
        }
    }
}

void FunctionAnalyser::analyseReturn(ASTBlock *root) {
    if (function_ == function_->package()->startFlagFunction() && function_->returnType()->type() == Type::noReturn()) {
        function_->setReturnType(std::make_unique<ASTLiteralType>(integer()));
        auto value = std::make_shared<ASTNumberLiteral>(static_cast<int64_t>(0), std::u32string(), root->position());
        root->appendNode(std::make_unique<ASTReturn>(value, root->position()));
        root->setReturnedCertainly();
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
        root->setReturnedCertainly();
    }
    else if (function_->functionType() == FunctionType::ValueTypeInitializer) {
        root->appendNode(std::make_unique<ASTReturn>(nullptr, root->position()));
        root->setReturnedCertainly();
    }
    else if (!pathAnalyser_.hasCertainly(PathAnalyserIncident::Returned)) {
        if (function_->returnType()->type().type() != TypeType::NoReturn) {
            compiler()->error(CompilerError(function_->position(), "An explicit return is missing."));
        }
        else {
            root->appendNode(std::make_unique<ASTReturn>(nullptr, root->position()));
            root->setReturnedCertainly();
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

}  // namespace EmojicodeCompiler
