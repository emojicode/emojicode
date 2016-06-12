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

std::list<Class *> Class::classes_;

Class::Class(EmojicodeChar name, Package *pkg, SourcePosition p, const EmojicodeString &documentation)
    : TypeDefinitionFunctional(name, pkg, p, documentation) {
    index = classes_.size();
    classes_.push_back(this);
}

bool Class::canBeUsedToResolve(TypeDefinitionFunctional *resolutionConstraint) {
    if (Class *cl = dynamic_cast<Class *>(resolutionConstraint)) {
        return inheritsFrom(cl);
    }
    return false;
}

bool Class::inheritsFrom(Class *from) {
    for (Class *a = this; a != nullptr; a = a->superclass) {
        if (a == from) {
            return true;
        }
    }
    return false;
}

void Class::addInstanceVariable(const Variable &variable) {
    instanceVariables_.push_back(variable);
}

bool Class::addProtocol(Type protocol) {
    protocols_.push_back(protocol);
    return true;
}

Initializer* Class::lookupInitializer(EmojicodeChar name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass) {
        auto pos = eclass->initializers_.find(name);
        if (pos != eclass->initializers_.end()) {
            return pos->second;
        }
        if (!eclass->inheritsContructors) {  // Does this eclass inherit initializers?
            break;
        }
    }
    return nullptr;
}

Method* Class::lookupMethod(EmojicodeChar name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass) {
        auto pos = eclass->methods_.find(name);
        if (pos != eclass->methods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

ClassMethod* Class::lookupClassMethod(EmojicodeChar name) {
    for (auto eclass = this; eclass != nullptr; eclass = eclass->superclass) {
        auto pos = eclass->classMethods_.find(name);
        if (pos != eclass->classMethods_.end()) {
            return pos->second;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name);
}