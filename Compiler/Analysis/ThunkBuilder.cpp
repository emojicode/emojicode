//
//  ThunkBuilder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ThunkBuilder.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTVariables.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Class.hpp"
#include "Types/Protocol.hpp"
#include <memory>

namespace EmojicodeCompiler {

void buildBoxingThunkAst(Function *layer, const Function *destinationFunction) {
    auto p = layer->position();
    auto args = ASTArguments(p);
    for (auto param : layer->parameters()) {
        args.addArguments(std::make_shared<ASTGetVariable>(param.name, p));
    }

    std::shared_ptr<ASTExpr> call;
    if (destinationFunction == nullptr) {
        call = std::make_shared<ASTCallableCall>(std::make_shared<ASTThis>(p), args, p);
    }
    else {
        call = std::make_shared<ASTMethod>(destinationFunction->name(), std::make_shared<ASTThis>(p), args, p);
    }

    std::shared_ptr<ASTBlock> block = std::make_shared<ASTBlock>(p);
    if (layer->returnType().type() == TypeType::NoReturn) {
        block->appendNode(std::make_shared<ASTExprStatement>(call, p));
    }
    else {
        block->appendNode(std::make_shared<ASTReturn>(call, p));
    }
    layer->setAst(block);
}

std::unique_ptr<Function> makeBoxingThunk(std::u32string name, const Type &owner, Package *package, SourcePosition p,
                                          std::vector<Parameter> &&params, const Type &returnType,
                                          FunctionType functionType) {
    auto function = std::make_unique<Function>(name, AccessLevel::Private, true, owner, package, p, false,
                                               std::u32string(), false, false, true, false, functionType);
    function->setParameters(std::move(params));
    function->setReturnType(returnType);
    return function;
}

std::unique_ptr<Function> buildBoxingThunk(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation) {
    auto params = std::vector<Parameter>();
    params.reserve(method->parameters().size());
    for (auto &arg : method->parameters()) {
        params.emplace_back(arg.name, arg.type.resolveOn(declarator));
    }

    auto name = declarator.calleeType().type() == TypeType::NoReturn ? std::u32string({ 'S' }) + methodImplementation->name()
        : methodImplementation->protocolBoxingThunk(declarator.calleeType().typeDefinition()->name());
    auto function = makeBoxingThunk(name, methodImplementation->owningType(), methodImplementation->package(),
                                    methodImplementation->position(), std::move(params),
                                    method->returnType().resolveOn(declarator), methodImplementation->functionType());
    buildBoxingThunkAst(function.get(), methodImplementation);
    return function;
}

std::unique_ptr<Function> buildBoxingThunk(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p) {
    auto params = std::vector<Parameter>();
    params.reserve(expectation.genericArguments().size() - 1);
    for (auto argumentType = expectation.genericArguments().begin() + 1;
        argumentType != expectation.genericArguments().end(); argumentType++) {
        params.emplace_back(std::u32string(1, expectation.genericArguments().end() - argumentType),
                               *argumentType);
    }

    auto function = makeBoxingThunk(std::u32string(), destCallable, pkg, p, std::move(params),
                                    expectation.genericArguments().front(), FunctionType::Closure);
    buildBoxingThunkAst(function.get(), nullptr);
    return function;
}

std::unique_ptr<Function> buildRequiredInitThunk(Class *klass, const Initializer *init) {
    auto name = std::u32string({ E_KEY }) + init->name();
    auto overriding = klass->superclass() != nullptr && klass->superclass()->lookupTypeMethod(name, true) != nullptr;
    auto function = std::make_unique<Function>(name, AccessLevel::Public, false, init->owningType(),
                                               init->package(), init->position(),
                                               overriding, std::u32string(), false, false, true, init->unsafe(),
                                               FunctionType::ClassMethod);

    function->setReturnType(init->constructedType(init->owningType()));
    function->setParameters(init->parameters());

    auto args = ASTArguments(init->position());
    for (auto param : function->parameters()) {
        args.addArguments(std::make_shared<ASTGetVariable>(param.name, init->position()));
    }
    auto type = std::make_shared<ASTStaticType>(init->owningType(), init->position());
    auto initCall = std::make_shared<ASTInitialization>(init->name(), type, args, init->position());
    std::shared_ptr<ASTBlock> block = std::make_shared<ASTBlock>(init->position());
    block->appendNode(std::make_shared<ASTReturn>(initCall, init->position()));
    function->setAst(block);
    return function;
}

}  // namespace EmojicodeCompiler
