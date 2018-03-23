//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Utils/StringUtils.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Class.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "TypeContext.hpp"
#include <algorithm>
#include <utility>

namespace EmojicodeCompiler {

Class::Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool exported,
             bool final, bool foreign) : TypeDefinition(std::move(name), pkg, std::move(p), documentation, exported),
                                         final_(final), foreign_(foreign) {
    instanceScope() = Scope(1);  // reassign a scoper with one offset for the pointer to the class meta
}

void Class::inherit(SemanticAnalyser *analyser) {
    if (superclass() == nullptr) {
        analyser->declareInstanceVariables(this);
        return;
    }

    if (instanceVariables().empty() && initializerList().empty()) {
        inheritsInitializers_ = true;
    }

    instanceScope() = superclass()->instanceScope();
    instanceScope().markInherited();

    analyser->declareInstanceVariables(this);

    Type classType = Type(this);
    instanceVariablesMut().insert(instanceVariables().begin(), superclass()->instanceVariables().begin(),
                                  superclass()->instanceVariables().end());

    protocols_.reserve(superclass()->protocols().size());
    for (auto &protocol : superclass()->protocols()) {
        auto find = std::find_if(protocols_.begin(), protocols_.end(), [&classType, &protocol](const Type &a) {
            return a.identicalTo(protocol, TypeContext(classType), nullptr);
        });
        if (find != protocols_.end()) {
            analyser->compiler()->error(CompilerError(position(), "Superclass already declared conformance to ",
                                                      protocol.toString(TypeContext(classType)), "."));
        }
        protocols_.emplace_back(protocol);
    }

    eachFunction([this, analyser](Function *function) {
        if (function->functionType() == FunctionType::ObjectInitializer) {
            checkInheritedRequiredInit(dynamic_cast<Initializer *>(function));
        }
        else {
            checkOverride(function, analyser);
        }
    });
}

void Class::checkOverride(Function *function, SemanticAnalyser *analyser) {
    auto superFunction = findSuperFunction(function);
    if (function->overriding()) {
        if (superFunction == nullptr || superFunction->accessLevel() == AccessLevel::Private) {
            analyser->compiler()->error(CompilerError(function->position(), utf8(function->name()),
                                                      " was declared ✒️ but does not override anything."));
        }
        analyser->enforcePromises(function, superFunction, Type(superclass()), TypeContext(Type(this)), TypeContext());
        superFunction->appointHeir(function);
    }
    else if (superFunction != nullptr && superFunction->accessLevel() != AccessLevel::Private) {
        analyser->compiler()->error(CompilerError(function->position(), "If you want to override ",
                                                  utf8(function->name()), " add ✒️."));
    }
}

void Class::checkInheritedRequiredInit(Initializer *initializer) {
    auto superInit = findSuperInitializer(initializer);
    if (initializer->required() && superInit != nullptr && superInit->required()) {
        superInit->appointHeir(initializer);
    }
}

template <typename FT>
FT* ifNotPrivate(FT *function) {
    if (function == nullptr) {
        return nullptr;
    }
    return function->accessLevel() == AccessLevel::Private ? nullptr : function;
}

Function* Class::findSuperFunction(Function *function) const {
    switch (function->functionType()) {
        case FunctionType::ObjectMethod:
        case FunctionType::BoxingLayer:
            return ifNotPrivate(superclass()->lookupMethod(function->name(), function->isImperative()));
        case FunctionType::ClassMethod:
            return ifNotPrivate(superclass()->lookupTypeMethod(function->name(), function->isImperative()));
        default:
            throw std::logic_error("Function of unexpected type in class");
    }
}

Initializer* Class::findSuperInitializer(Initializer *function) const {
    return ifNotPrivate(superclass()->lookupInitializer(function->name()));
}

void Class::addInstanceVariable(const InstanceVariableDeclaration &declaration) {
    if (foreign()) {
        throw CompilerError(position(), "Instance variable is not allowed in foreign class.");
    }
    TypeDefinition::addInstanceVariable(declaration);
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

Function * Class::lookupMethod(const std::u32string &name, bool imperative) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupMethod(name, imperative)) {
            return method;
        }
    }
    return nullptr;
}

Function * Class::lookupTypeMethod(const std::u32string &name, bool imperative) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupTypeMethod(name, imperative)) {
            return method;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name());
}


}  // namespace EmojicodeCompiler
