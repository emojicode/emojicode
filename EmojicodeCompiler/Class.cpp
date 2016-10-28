//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <vector>
#include <map>
#include <utility>
#include "EmojicodeCompiler.hpp"
#include "CompilerErrorException.hpp"
#include "Class.hpp"
#include "Function.hpp"
#include "utf8.h"
#include "Lexer.hpp"
#include "TypeContext.hpp"
#include "Protocol.hpp"

std::list<Class *> Class::classes_;

Class::Class(EmojicodeChar name, Package *pkg, SourcePosition p, const EmojicodeString &documentation, bool final)
    : TypeDefinitionFunctional(name, pkg, p, documentation) {
    index = classes_.size();
    final_ = final;
    classes_.push_back(this);
}

void Class::setSuperclass(Class *eclass) {
    superclass_ = eclass;
}

bool Class::canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) {
    if (Class *cl = dynamic_cast<Class *>(resolutionConstraint)) {
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

void Class::addInstanceVariable(const Variable &variable) {
    instanceVariables_.push_back(variable);
}

void Class::addProtocol(Type protocol) {
    protocols_.push_back(protocol);
}

Initializer* Class::lookupInitializer(EmojicodeChar name) {
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

Function* Class::lookupMethod(EmojicodeChar name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass()) {
        auto pos = eclass->methods_.find(name);
        if (pos != eclass->methods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

Function* Class::lookupClassMethod(EmojicodeChar name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass()) {
        auto pos = eclass->classMethods_.find(name);
        if (pos != eclass->classMethods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name());
}

void Class::finalize() {
    if (instanceVariables().size() == 0 && initializerList().size() == 0) {
        inheritsInitializers_ = true;
    }
    
    Type classType = Type(this);
    
    if (superclass()) {
        for (auto method : superclass()->methodList()) {
            method->assignVti();
        }
        for (auto clMethod : superclass()->classMethodList()) {
            clMethod->assignVti();
        }
        for (auto initializer : superclass()->initializerList()) {
            initializer->assignVti();
        }
    }
    
    for (auto method : methodList()) {
        auto superMethod = superclass() ? superclass()->lookupMethod(method->name()) : NULL;
        
        method->setVtiProvider(&methodVtiProvider_);
        if (method->checkOverride(superMethod)) {
            method->checkPromises(superMethod, "super method", classType);
            method->setVti(superMethod->getVti());
            superMethod->registerOverrider(method);
            methodVtiProvider_.incrementVtiCount();
        }
    }
    for (auto clMethod : classMethodList()) {
        auto superMethod = superclass() ? superclass()->lookupClassMethod(clMethod->name()) : NULL;
        
        clMethod->setVtiProvider(&methodVtiProvider_);
        if (clMethod->checkOverride(superMethod)) {
            clMethod->checkPromises(superMethod, "super classmethod", classType);
            clMethod->setVti(superMethod->getVti());
            superMethod->registerOverrider(clMethod);
            methodVtiProvider_.incrementVtiCount();
        }
    }
    
    auto subRequiredInitializerNextVti = superclass() ? superclass()->requiredInitializers().size() : 0;
    for (auto initializer : initializerList()) {
        auto superInit = superclass() ? superclass()->lookupInitializer(initializer->name()) : NULL;
        
        initializer->setVtiProvider(&initializerVtiProvider_);
        if (initializer->required) {
            if (superInit && superInit->required) {
                initializer->checkPromises(superInit, "super initializer", classType);
                initializer->setVti(superInit->getVti());
                superInit->registerOverrider(initializer);
                initializerVtiProvider_.incrementVtiCount();
            }
            else {
                initializer->setVti(static_cast<int>(subRequiredInitializerNextVti++));
                initializerVtiProvider_.incrementVtiCount();
            }
        }
    }
    
    if (superclass()) {
        initializerVtiProvider_.offsetVti(inheritsInitializers() ? superclass()->initializerVtiProvider_.peekNext() :
                                          static_cast<int>(requiredInitializers().size()));
        methodVtiProvider_.offsetVti(superclass()->methodVtiProvider_.peekNext());
        nextInstanceVariableID_ = superclass()->nextInstanceVariableID_;
    }
    
    for (Variable var : instanceVariables()) {
        var.setId(static_cast<int>(nextInstanceVariableID_++));
        objectScope_.setLocalVariable(var.definitionToken.value, var);
    }
    
    if (nextInstanceVariableID_ > 65536) {
        throw CompilerErrorException(position(), "You exceeded the limit of 65,536 instance variables.");
    }
    
    if (instanceVariables().size() > 0 && initializerList().size() == 0) {
        auto str = classType.toString(typeNothingness, true);
        compilerWarning(position(), "Class %s defines %d instances variables but has no initializers.",
                        str.c_str(), instanceVariables().size());
    }
    
    for (Type protocol : protocols()) {
        for (auto method : protocol.protocol()->methods()) {
            Function *clm = lookupMethod(method->name());
            if (clm) {
                clm->assignVti();
                method->registerOverrider(clm);
            }
        }
    }
}
