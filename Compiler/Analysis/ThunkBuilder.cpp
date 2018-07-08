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
#include "Emojis.h"
#include <memory>

namespace EmojicodeCompiler {

void buildBoxingThunkAst(Function *layer, const Function *destinationFunction) {
    auto p = layer->position();
    auto args = ASTArguments(p);
    for (auto &param : layer->parameters()) {
        args.addArguments(std::make_shared<ASTGetVariable>(param.name, p));
    }

    std::shared_ptr<ASTExpr> call;
    if (destinationFunction == nullptr) {
        call = std::make_shared<ASTCallableCall>(std::make_shared<ASTThis>(p), args, p);
    }
    else {
        call = std::make_shared<ASTMethod>(destinationFunction->name(), std::make_shared<ASTThis>(p), args, p);
    }

    auto block = std::make_unique<ASTBlock>(p);
    if (layer->returnType()->type().type() == TypeType::NoReturn) {
        block->appendNode(std::make_unique<ASTExprStatement>(call, p));
    }
    else {
        block->appendNode(std::make_unique<ASTReturn>(call, p));
    }
    layer->setAst(std::move(block));
}

std::unique_ptr<Function> makeBoxingThunk(std::u32string name, TypeDefinition *owner, Package *package,
                                          SourcePosition p, std::vector<Parameter> &&params, const Type &returnType,
                                          FunctionType functionType) {
    auto function = std::make_unique<Function>(name, AccessLevel::Private, true, owner, package, p, false,
                                               std::u32string(), false, false, true, false, functionType);
    function->setParameters(std::move(params));
    function->setReturnType(std::make_unique<ASTLiteralType>(returnType));
    return function;
}

std::unique_ptr<Function> buildBoxingThunk(const TypeContext &declarator, const Function *method,
                                           const Function *methodImplementation) {
    auto params = std::vector<Parameter>();
    params.reserve(method->parameters().size());
    for (auto &arg : method->parameters()) {
        params.emplace_back(arg.name, std::make_unique<ASTLiteralType>(arg.type->type().resolveOn(declarator)),
                            MFType::Borrowing);
    }

    auto name = declarator.calleeType().type() == TypeType::NoReturn ? std::u32string({ 'S' }) + methodImplementation->name()
        : methodImplementation->protocolBoxingThunk(declarator.calleeType().typeDefinition()->name());
    auto function = makeBoxingThunk(name, methodImplementation->owner(), methodImplementation->package(),
                                    methodImplementation->position(), std::move(params),
                                    method->returnType()->type().resolveOn(declarator),
                                    methodImplementation->functionType());
    buildBoxingThunkAst(function.get(), methodImplementation);
    return function;
}

std::unique_ptr<Function> buildBoxingThunk(const TypeExpectation &expectation, const Type &destCallable,
                                           Package *pkg, const SourcePosition &p) {
    auto params = std::vector<Parameter>();
    params.reserve(expectation.genericArguments().size());
    params.emplace_back(U"closure", std::make_unique<ASTLiteralType>(destCallable), MFType::Escaping);
    for (auto argumentType = expectation.genericArguments().begin() + 1;
         argumentType != expectation.genericArguments().end(); argumentType++) {
        params.emplace_back(std::u32string(1, expectation.genericArguments().end() - argumentType),
                            std::make_unique<ASTLiteralType>(*argumentType), MFType::Borrowing);
    }

    auto function = makeBoxingThunk(std::u32string(), nullptr, pkg, p, std::move(params),
                                    expectation.genericArguments().front(), FunctionType::Function);
    function->setClosure();
    buildBoxingThunkAst(function.get(), nullptr);
    return function;
}

std::unique_ptr<Function> buildRequiredInitThunk(Class *klass, const Initializer *init) {
    auto name = std::u32string({ E_KEY }) + init->name();
    auto overriding = klass->superclass() != nullptr && klass->superclass()->lookupTypeMethod(name, true) != nullptr;
    auto function = std::make_unique<Function>(name, AccessLevel::Public, false, init->owner(),
                                               init->package(), init->position(),
                                               overriding, std::u32string(), false, false, true, init->unsafe(),
                                               FunctionType::ClassMethod);

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
