//
//  ThunkBuilder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ThunkBuilder.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTVariables.hpp"
#include "Emojis.h"
#include "Functions/Initializer.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeExpectation.hpp"
#include "Types/TypeContext.hpp"
#include <memory>

namespace EmojicodeCompiler {

void buildBoxingThunkAst(Function *thunk, const Function *destinationFunction, const Type &destCallable) {
    auto p = thunk->position();
    auto args = ASTArguments(p);
    for (auto &param : thunk->parameters()) {
        args.addArguments(std::make_shared<ASTGetVariable>(param.name, p));
    }

    std::shared_ptr<ASTExpr> call;
    if (destinationFunction == nullptr) {
        call = std::make_shared<ASTCallableCall>(std::make_shared<ASTCallableThunkDestination>(p, destCallable),
                                                 args, p);
    }
    else {
        call = std::make_shared<ASTMethod>(destinationFunction->name(), std::make_shared<ASTThis>(p), args, p);
    }

    auto block = std::make_unique<ASTBlock>(p);
    if (thunk->returnType()->type().type() == TypeType::NoReturn) {
        block->appendNode(std::make_unique<ASTExprStatement>(call, p));
    }
    else {
        block->appendNode(std::make_unique<ASTReturn>(call, p));
    }
    thunk->setAst(std::move(block));
}

std::unique_ptr<Function> makeBoxingThunk(std::u32string name, TypeDefinition *owner, Package *package,
                                          SourcePosition p, std::vector<Parameter> &&params, const Type &returnType,
                                          FunctionType functionType) {
    auto function = std::make_unique<Function>(name, AccessLevel::Private, true, owner, package, p, false,
                                               std::u32string(), false, false, Mood::Imperative, false, functionType,
                                               false);
    function->setParameters(std::move(params));
    function->setReturnType(std::make_unique<ASTLiteralType>(returnType));
    function->setThunk();
    return function;
}

std::unique_ptr<Function> buildBoxingThunk(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation) {
    auto params = std::vector<Parameter>();
    params.reserve(method->parameters().size());
    for (auto &arg : method->parameters()) {
        params.emplace_back(arg.name, std::make_unique<ASTLiteralType>(arg.type->type().resolveOn(declarator)),
                            MFFlowCategory::Borrowing);
    }

    auto name = declarator.calleeType().type() == TypeType::NoReturn ? std::u32string({ 'S' }) + methodImplementation->name()
        : methodImplementation->protocolBoxingThunk(declarator.calleeType().typeDefinition()->name());
    auto function = makeBoxingThunk(name, methodImplementation->owner(), methodImplementation->package(),
                                    methodImplementation->position(), std::move(params),
                                    method->returnType()->type().resolveOn(declarator),
                                    methodImplementation->functionType());
    buildBoxingThunkAst(function.get(), methodImplementation, Type::noReturn());
    return function;
}

std::unique_ptr<Function> buildCallableThunk(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p) {
    auto params = std::vector<Parameter>();
    params.reserve(expectation.parametersCount());
    for (auto paramType = expectation.parameters(); paramType != expectation.parametersEnd(); paramType++) {
        params.emplace_back(std::u32string(1, 'A' + expectation.parameters() - paramType),
                            std::make_unique<ASTLiteralType>(*paramType), MFFlowCategory::Borrowing);
    }

    auto function = makeBoxingThunk(std::u32string(), nullptr, pkg, p, std::move(params),
                                    expectation.returnType(), FunctionType::Function);
    function->setClosure();
    buildBoxingThunkAst(function.get(), nullptr, destCallable);
    return function;
}

std::unique_ptr<Function> buildRequiredInitThunk(Class *klass, const Initializer *init) {
    auto name = std::u32string({ E_KEY }) + init->name();
    auto overriding = klass->superclass() != nullptr &&
                      klass->superclass()->lookupTypeMethod(name, Mood::Imperative) != nullptr;
    auto function = std::make_unique<Function>(name, AccessLevel::Public, false, init->owner(),
                                               init->package(), init->position(),
                                               overriding, std::u32string(), false, false, Mood::Imperative,
                                               init->unsafe(), FunctionType::ClassMethod, false);
    function->setThunk();

    function->setReturnType(std::make_unique<ASTLiteralType>(init->constructedType(init->owner()->type())));
    function->setParameters(init->parameters());

    auto args = ASTArguments(init->position());
    for (auto &param : function->parameters()) {
        args.addArguments(std::make_shared<ASTGetVariable>(param.name, init->position()));
    }
    auto type = std::make_shared<ASTStaticType>(std::make_unique<ASTLiteralType>(init->owner()->type()),
                                                init->position());
    auto initCall = std::make_shared<ASTInitialization>(init->name(), type, args, init->position());
    auto block = std::make_unique<ASTBlock>(init->position());
    block->appendNode(std::make_unique<ASTReturn>(initCall, init->position()));
    function->setAst(std::move(block));
    return function;
}

}  // namespace EmojicodeCompiler
