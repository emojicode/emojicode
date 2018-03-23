//
// Created by Theo Weidmann on 26.02.18.
//

#include "BoxingLayerBuilder.hpp"
#include "Compiler.hpp"
#include "FunctionAnalyser.hpp"
#include "Functions/BoxingLayer.hpp"
#include "SemanticAnalyser.hpp"
#include "Types/Class.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/TypeDefinition.hpp"
#include "Types/ValueType.hpp"
#include <experimental/optional>
#include <iostream>

namespace EmojicodeCompiler {

Compiler* SemanticAnalyser::compiler() const {
    return package_->compiler();
}

void SemanticAnalyser::analyse(bool executable) {
    for (auto &extension : package_->extensions()) {
        extension->extend();
    }
    for (auto &vt : package_->valueTypes()) {
        finalizeProtocols(Type(vt.get()));
        declareInstanceVariables(vt.get());
        enqueueFunctionsOfTypeDefinition(vt.get());
    }
    for (auto &eclass : package_->classes()) {
        eclass->inherit(this);
        finalizeProtocols(Type(eclass.get()));
        enqueueFunctionsOfTypeDefinition(eclass.get());
    }
    for (auto &function : package_->functions()) {
        enqueueFunction(function.get());
    }

    analyseQueue();

    if (executable && !package_->hasStartFlagFunction()) {
        compiler()->error(CompilerError(package_->position(), "No 🏁 block was found."));
    }
}

void SemanticAnalyser::analyseQueue() {
    while (!queue_.empty()) {
        try {
            FunctionAnalyser(queue_.front(), this).analyse();
        }
        catch (CompilerError &ce) {
            package_->compiler()->error(ce);
        }
        queue_.pop();
    }
}

void SemanticAnalyser::enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef) {
    typeDef->eachFunction([this](Function *function) {
        enqueueFunction(function);
    });
}

void SemanticAnalyser::enqueueFunction(Function *function) {
    if (!function->isExternal()) {
        queue_.emplace(function);
    }
}

void SemanticAnalyser::declareInstanceVariables(TypeDefinition *typeDef) {
    for (auto &var : typeDef->instanceVariables()) {
        typeDef->instanceScope().declareVariable(var.name, var.type, false, var.position);
    }

    if (!typeDef->instanceVariables().empty() && typeDef->initializerList().empty()) {
        package_->compiler()->warn(typeDef->position(), "Type defines ", typeDef->instanceVariables().size(),
                                   " instances variables but has no initializers.");
    }
}

bool SemanticAnalyser::enforcePromises(const Function *sub, const Function *super, const Type &superSource,
                                       const TypeContext &subContext, const TypeContext &superContext) {
    if (super->final()) {
        package_->compiler()->error(CompilerError(sub->position(), superSource.toString(subContext),
                                                  "’s implementation of ", utf8(sub->name()), " was marked 🔏."));
    }



    if (sub->accessLevel() == AccessLevel::Private || (sub->accessLevel() == AccessLevel::Protected &&
            super->accessLevel() == AccessLevel::Public)) {
        package_->compiler()->error(CompilerError(sub->position(), "Overriding method must be as accessible or more ",
                                                  "accessible than the overridden method."));
    }

    auto superReturnType = super->returnType().resolveOn(superContext);
    if (!sub->returnType().resolveOn(subContext).compatibleTo(superReturnType, subContext)) {
        auto supername = superReturnType.toString(subContext);
        auto thisname = sub->returnType().toString(subContext);
        package_->compiler()->error(CompilerError(sub->position(), "Return type ",
                                                  sub->returnType().toString(subContext), " of ", utf8(sub->name()),
                                                  " is not compatible to the return type defined in ",
                                                  superSource.toString(subContext)));
    }
    if (sub->returnType().resolveOn(subContext).storageType() != superReturnType.storageType()) {
        return false;  // BoxingLayer required for the return type
    }

    return checkArgumentPromise(sub, super, subContext, superContext);
}

bool SemanticAnalyser::checkArgumentPromise(const Function *sub, const Function *super, const TypeContext &subContext,
                                            const TypeContext &superContext) const {
    if (super->parameters().size() != sub->parameters().size()) {
        package_->compiler()->error(CompilerError(sub->position(), "Parameter count does not match."));
    }

    bool compatible = true;
    for (size_t i = 0; i < super->parameters().size(); i++) { // More general arguments are OK
        auto superArgumentType = super->parameters()[i].type.resolveOn(superContext);
        if (!superArgumentType.compatibleTo(sub->parameters()[i].type.resolveOn(subContext), subContext)) {
            auto supertype = superArgumentType.toString(subContext);
            auto thisname = sub->parameters()[i].type.toString(subContext);
            package_->compiler()->error(CompilerError(sub->position(), "Type ", thisname, " of argument ", i + 1,
                                                      " is not compatible with its ", thisname, " argument type ",
                                                      supertype, "."));
        }
        if (sub->parameters()[i].type.resolveOn(subContext).storageType() != superArgumentType.storageType()) {
            compatible = false;  // BoxingLayer required for parameter i
        }
    }
    return compatible;
}

void SemanticAnalyser::finalizeProtocol(const Type &type, const Type &protocol) {
    for (auto method : protocol.protocol()->methodList()) {
        auto methodImplementation = type.typeDefinition()->lookupMethod(method->name(), method->isImperative());
        if (methodImplementation == nullptr) {
            package_->compiler()->error(
                    CompilerError(type.typeDefinition()->position(), type.toString(TypeContext()),
                                  " does not conform to protocol ", protocol.toString(TypeContext()), ": Method ",
                                  utf8(method->name()), " not provided."));
            continue;
        }

        methodImplementation->createUnspecificReification();
        if (enforcePromises(methodImplementation, method, protocol, TypeContext(type), TypeContext(protocol))) {
            method->appointHeir(methodImplementation);
        }
        else {
            buildBoxingLayer(type, protocol, method, methodImplementation);
        }
    }
}

void SemanticAnalyser::buildBoxingLayer(const Type &type, const Type &protocol, Function *method,
                                        Function *methodImplementation) {
    auto arguments = std::vector<Parameter>();
    arguments.reserve(method->parameters().size());
    for (auto &arg : method->parameters()) {
        arguments.emplace_back(arg.name, arg.type.resolveOn(TypeContext(protocol)));
    }
    auto bl = std::make_unique<BoxingLayer>(methodImplementation, protocol.protocol()->name(), arguments,
                                            method->returnType().resolveOn(TypeContext(protocol)),
                                            methodImplementation->position());
    buildBoxingLayerAst(bl.get());
    enqueueFunction(bl.get());
    type.typeDefinition()->addMethod(move(bl));
    method->appointHeir(bl.get());
}

void SemanticAnalyser::finalizeProtocols(const Type &type) {
    for (const Type &protocol : type.typeDefinition()->protocols()) {
        finalizeProtocol(type, protocol);
    }
}

}  // namespace EmojicodeCompiler