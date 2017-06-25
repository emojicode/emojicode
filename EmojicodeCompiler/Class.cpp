//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.hpp"
#include "Class.hpp"
#include "CompilerError.hpp"
#include "Function.hpp"
#include "Lexer.hpp"
#include "TypeContext.hpp"
#include <algorithm>
#include <map>
#include <utility>
#include <vector>

std::vector<Class *> Class::classes_;

Class::Class(EmojicodeString name, Package *pkg, SourcePosition p, const EmojicodeString &documentation, bool final)
: TypeDefinitionFunctional(name, pkg, p, documentation) {
    index = classes_.size();
    final_ = final;
    classes_.push_back(this);
}

void Class::setSuperclass(Class *eclass) {
    superclass_ = eclass;
}

bool Class::canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) const {
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

void Class::finalize() {
    Type classType = Type(this, false);

    if (superclass() != nullptr) {
        scoper_ = superclass()->scoper_;
        instanceScope().markInherited();
        protocols_.reserve(superclass()->protocols().size());
        for (auto &protocol : superclass()->protocols()) {
            auto find = std::find_if(protocols_.begin(), protocols_.end(), [&classType, &protocol](const Type &a) {
                return a.identicalTo(protocol, classType, nullptr);
            });
            if (find != protocols_.end()) {
                throw CompilerError(position(), "Superclass already declared conformance to %s.",
                                    protocol.toString(classType, true).c_str());
            }
            protocols_.emplace_back(protocol);
        }
    }

    TypeDefinitionFunctional::finalize();

    if (instanceVariables().empty() && initializerList().empty()) {
        inheritsInitializers_ = true;
    }

    if (superclass() != nullptr) {
        for (auto method : superclass()->methodList()) {
            method->assignVti();
        }
        for (auto clMethod : superclass()->typeMethodList()) {
            clMethod->assignVti();
        }
        for (auto initializer : superclass()->initializerList()) {
            initializer->assignVti();
        }
    }

    for (auto method : methodList()) {
        auto superMethod = superclass() != nullptr ? superclass()->lookupMethod(method->name()) : nullptr;

        method->setVtiProvider(&methodVtiProvider_);
        if (method->checkOverride(superMethod)) {
            method->override(superMethod, classType, classType);
            methodVtiProvider_.incrementVtiCount();
        }
    }
    for (auto clMethod : typeMethodList()) {
        auto superMethod = superclass() != nullptr ? superclass()->lookupTypeMethod(clMethod->name()) : nullptr;

        clMethod->setVtiProvider(&methodVtiProvider_);
        if (clMethod->checkOverride(superMethod)) {
            clMethod->override(superMethod, classType, classType);
            methodVtiProvider_.incrementVtiCount();
        }
    }

    auto subRequiredInitializerNextVti = superclass() != nullptr ? superclass()->requiredInitializers().size() : 0;
    for (auto initializer : initializerList()) {
        auto superInit = superclass() != nullptr ? superclass()->lookupInitializer(initializer->name()) : nullptr;

        initializer->setVtiProvider(&initializerVtiProvider_);
        if (initializer->required()) {
            if (superInit != nullptr && superInit->required()) {
                initializer->override(superInit, classType, classType);
                initializerVtiProvider_.incrementVtiCount();
            }
            else {
                initializer->setVti(static_cast<int>(subRequiredInitializerNextVti++));
                initializerVtiProvider_.incrementVtiCount();
            }
        }
    }

    if (superclass() != nullptr) {
        initializerVtiProvider_.offsetVti(inheritsInitializers() ? superclass()->initializerVtiProvider_.peekNext() :
                                          static_cast<int>(requiredInitializers().size()));
        methodVtiProvider_.offsetVti(superclass()->methodVtiProvider_.peekNext());
    }

    TypeDefinitionFunctional::finalizeProtocols(classType, &methodVtiProvider_);
}
