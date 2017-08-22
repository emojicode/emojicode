//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "../EmojicodeCompiler.hpp"
#include "../CompilerError.hpp"
#include "../Function.hpp"
#include "../Initializer.hpp"
#include "Class.hpp"
#include "TypeContext.hpp"
#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

std::vector<Class *> Class::classes_;

Class::Class(EmojicodeString name, Package *pkg, SourcePosition p, const EmojicodeString &documentation, bool final)
: TypeDefinition(std::move(name), pkg, std::move(p), documentation) {
    index = classes_.size();
    final_ = final;
    classes_.push_back(this);
}

void Class::setSuperclass(Class *eclass) {
    superclass_ = eclass;
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

Initializer* Class::lookupInitializer(const EmojicodeString &name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass()) {
        auto pos = eclass->initializers_.find(name);
        if (pos != eclass->initializers_.end()) {
            return pos->second;
        }
        if (!eclass->inheritsInitializers()) {
            break;
        }
    }
    return nullptr;
}

Function* Class::lookupMethod(const EmojicodeString &name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass()) {
        auto pos = eclass->methods_.find(name);
        if (pos != eclass->methods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

Function* Class::lookupTypeMethod(const EmojicodeString &name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass()) {
        auto pos = eclass->typeMethods_.find(name);
        if (pos != eclass->typeMethods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name());
}

void Class::prepareForSemanticAnalysis() {
    Type classType = Type(this, false);

    if (superclass() != nullptr) {
        scope_ = superclass()->scope_;
        instanceScope().markInherited();
    }

    TypeDefinition::prepareForSemanticAnalysis();

    if (instanceVariables().empty() && initializerList().empty()) {
        inheritsInitializers_ = true;
    }

    if (superclass() != nullptr) {
        protocols_.reserve(superclass()->protocols().size());
        for (auto &protocol : superclass()->protocols()) {
            auto find = std::find_if(protocols_.begin(), protocols_.end(), [&classType, &protocol](const Type &a) {
                return a.identicalTo(protocol, classType, nullptr);
            });
            if (find != protocols_.end()) {
                throw CompilerError(position(), "Superclass already declared conformance to %s.",
                                    protocol.toString(classType).c_str());
            }
            protocols_.emplace_back(protocol);
        }

        eachFunctionWithoutInitializers([this](Function *function) {
            checkOverride(function);
        });
    }

    TypeDefinition::finalizeProtocols(classType);
}

void Class::createCGScope() {
    if (superclass() != nullptr) {
        cgScoper_ = superclass()->cgScoper_;
        cgScoper_.resizeVariables(scope_.maxVariableId());
    }
    else {
        TypeDefinition::createCGScope();
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

void Class::checkOverride(Function *function) {
    auto superFunction = findSuperFunction(function);
    if (function->overriding()) {
        if (superFunction == nullptr || superFunction->accessLevel() == AccessLevel::Private) {
            throw CompilerError(function->position(), "%s was declared ✒️ but does not override anything.",
                                function->name().utf8().c_str());
        }
        function->enforcePromises(superFunction, Type(this, false), Type(superclass(), false),
                                  std::experimental::nullopt);
    }
    else if (superFunction != nullptr && superFunction->accessLevel() != AccessLevel::Private) {
        throw CompilerError(function->position(), "If you want to override %s add ✒️.", function->name().utf8().c_str());
    }
}

void Class::overrideFunction(Function *function) {
    if (superclass() != nullptr) {
        if (auto superFunction = findSuperFunction(function)) {
            function->setVti(superFunction->getVti());
            superFunction->registerOverrider(function);
        }
    }
}

void Class::prepareForCG() {
    Type classType = Type(this, false);

    TypeDefinition::prepareForCG();

    if (superclass() != nullptr) {
        superclass()->eachFunction([this](auto *function) {
            function->assignVti();
        });
    }

    eachFunctionWithoutInitializers([this](Function *function) {
        function->setVtiProvider(&methodVtiProvider_);
        overrideFunction(function);
    });

    auto subRequiredInitializerNextVti = superclass() != nullptr ? superclass()->requiredInitializers().size() : 0;
    for (auto initializer : initializerList()) {
        initializer->setVtiProvider(&initializerVtiProvider_);
        if (initializer->required()) {
            auto superInit = superclass() != nullptr ? findSuperFunction(initializer) : nullptr;
            if (superInit != nullptr && superInit->required()) {
                initializer->setVti(superInit->getVti());
                superInit->registerOverrider(initializer);
            }
            else {
                initializer->setVti(static_cast<int>(subRequiredInitializerNextVti++));
            }
        }
    }

    if (superclass() != nullptr) {
        initializerVtiProvider_.offsetVti(inheritsInitializers() ? superclass()->initializerVtiProvider_.peekNext() :
                                          static_cast<int>(requiredInitializers().size()));
        methodVtiProvider_.offsetVti(superclass()->methodVtiProvider_.peekNext());
    }
}

}  // namespace EmojicodeCompiler
