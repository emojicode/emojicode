//
//  BoxingLayerBuilder.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 14/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "BoxingLayerBuilder.hpp"
#include "Types/Protocol.hpp"
#include "AST/ASTExpr.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTVariables.hpp"
#include "Functions/Function.hpp"
#include <memory>

namespace EmojicodeCompiler {

void buildBoxingLayerAst(Function *layer, const Function *destinationFunction) {
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

std::unique_ptr<Function> makeBoxingLayer(std::u32string name, const Type &owner, Package *package, SourcePosition p,
                                          std::vector<Parameter> &&params, Type returnType) {
    auto function = std::make_unique<Function>(name, AccessLevel::Private, true, owner, package, p, false,
                                               std::u32string(), false, false, true, false, FunctionType::BoxingLayer);
    function->setParameters(std::move(params));
    function->setReturnType(returnType);
    return function;
}

std::unique_ptr<Function> buildBoxingLayer(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation) {
    auto params = std::vector<Parameter>();
    params.reserve(method->parameters().size());
    for (auto &arg : method->parameters()) {
        params.emplace_back(arg.name, arg.type.resolveOn(declarator));
    }

    auto name = declarator.calleeType().type() == TypeType::NoReturn ? std::u32string()
        : methodImplementation->protocolBoxingLayerName(declarator.calleeType().typeDefinition()->name());
    auto function = makeBoxingLayer(name, methodImplementation->owningType(), methodImplementation->package(),
                                    methodImplementation->position(), std::move(params),
                                    method->returnType().resolveOn(declarator));
    buildBoxingLayerAst(function.get(), methodImplementation);
    return function;
}

std::unique_ptr<Function> buildBoxingLayer(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p) {
    auto params = std::vector<Parameter>();
    params.reserve(expectation.genericArguments().size() - 1);
    for (auto argumentType = expectation.genericArguments().begin() + 1;
        argumentType != expectation.genericArguments().end(); argumentType++) {
        params.emplace_back(std::u32string(1, expectation.genericArguments().end() - argumentType),
                               *argumentType);
    }

    auto function = makeBoxingLayer(std::u32string(), destCallable, pkg, p, std::move(params),
                                    expectation.genericArguments().front());
    buildBoxingLayerAst(function.get(), nullptr);
    return function;
}

}  // namespace EmojicodeCompiler
