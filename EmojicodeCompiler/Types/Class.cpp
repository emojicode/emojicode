//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "../EmojicodeCompiler.hpp"
#include "../Application.hpp"
#include "../CompilerError.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"
#include "Class.hpp"
#include "TypeContext.hpp"
#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

Class::Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool final)
: TypeDefinition(std::move(name), pkg, std::move(p), documentation), final_(final) {
    instanceScope() = Scope(1);  // reassign a scoper with one offset for the pointer to the class meta
}

void Class::prepareForSemanticAnalysis() {
    Type classType = Type(this, false);

    if (superclass() != nullptr) {
        instanceScope() = superclass()->instanceScope();
        instanceScope().markInherited();
    }

    TypeDefinition::prepareForSemanticAnalysis();

    if (instanceVariables().empty() && initializerList().empty()) {
        inheritsInitializers_ = true;
    }

    if (superclass() != nullptr) {
        instanceVariablesMut().insert(instanceVariables().begin(), superclass()->instanceVariables().begin(),
                                      superclass()->instanceVariables().end());

        protocols_.reserve(superclass()->protocols().size());
        for (auto &protocol : superclass()->protocols()) {
            auto find = std::find_if(protocols_.begin(), protocols_.end(), [&classType, &protocol](const Type &a) {
                return a.identicalTo(protocol, TypeContext(classType), nullptr);
            });
            if (find != protocols_.end()) {
                throw CompilerError(position(), "Superclass already declared conformance to ",
                                    protocol.toString(TypeContext(classType)), ".");
            }
            protocols_.emplace_back(protocol);
        }

        eachFunctionWithoutInitializers([this](Function *function) {
            if (function->functionType() == FunctionType::ObjectInitializer) {
                checkInheritedRequiredInit(dynamic_cast<Initializer *>(function));
            }
            else {
                checkOverride(function);
            }
        });

        virtualTableIndex_ = superclass()->virtualTableIndex_;
    }

    TypeDefinition::finalizeProtocols(classType);

    eachFunction([this](Function *function) {
        if (function->functionType() == FunctionType::ObjectInitializer) {
            auto init = dynamic_cast<Initializer *>(function);
            if (!init->required()) {
                return;
            }
        }
        if (!function->hasVti()) {
            function->setVti(virtualTableIndex_++);
        }
    });
}

void Class::checkOverride(Function *function) {
    auto superFunction = findSuperFunction(function);
    if (function->overriding()) {
        if (superFunction == nullptr || superFunction->accessLevel() == AccessLevel::Private) {
            throw CompilerError(function->position(), utf8(function->name()),
                                " was declared ✒️ but does not override anything.");
        }
        function->enforcePromises(superFunction, TypeContext(Type(this, false)), Type(superclass(), false),
                                  std::experimental::nullopt);
        function->setVti(superFunction->vti());
        superFunction->registerOverrider(function);
    }
    else if (superFunction != nullptr && superFunction->accessLevel() != AccessLevel::Private) {
        throw CompilerError(function->position(), "If you want to override ", utf8(function->name()), " add ✒️.");
    }
}

void Class::checkInheritedRequiredInit(Initializer *initializer) {
    auto superInit = findSuperFunction(initializer);
    if (initializer->required() && superInit != nullptr && superInit->required()) {
        initializer->setVti(superInit->vti());
        superInit->registerOverrider(initializer);
    }
}

Function* Class::findSuperFunction(Function *function) const {
    switch (function->functionType()) {
        case FunctionType::ObjectMethod:
        case FunctionType::BoxingLayer:
            return superclass()->lookupMethod(function->name());
        case FunctionType::ClassMethod:
            return superclass()->lookupTypeMethod(function->name());
        default:
            throw std::logic_error("Function of unexpected type in class");
    }
}

Initializer* Class::findSuperFunction(Initializer *function) const {
    return superclass()->lookupInitializer(function->name());
}

bool Class::canBeUsedToResolve(TypeDefinition *resolutionConstraint) const {
    if (auto cl = dynamic_cast<Class *>(resolutionConstraint)) {
        return inheritsFrom(cl);
    }
    return false;
}

bool Class::inheritsFrom(Class *from) const {
    for (const Class *a = this; a != nullptr; a = a->superclass()) {
        if (a == from) {
            return true;
        }
    }
    return false;
}

Initializer* Class::lookupInitializer(const std::u32string &name) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto initializer = klass->TypeDefinition::lookupInitializer(name)) {
            return initializer;
        }
        if (!klass->inheritsInitializers()) {
            break;
        }
    }
    return nullptr;
}

Function* Class::lookupMethod(const std::u32string &name) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupMethod(name)) {
            return method;
        }
    }
    return nullptr;
}

Function* Class::lookupTypeMethod(const std::u32string &name) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupTypeMethod(name)) {
            return method;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name());
}


}  // namespace EmojicodeCompiler
