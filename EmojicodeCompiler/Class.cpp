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
#include "Procedure.hpp"
#include "utf8.h"
#include "Lexer.hpp"
#include "TypeContext.hpp"

std::list<Class *> Class::classes_;

Class::Class(EmojicodeChar name, SourcePosition p, Package *pkg, const EmojicodeString &documentation)
    : TypeDefinitionWithGenerics(name, pkg, documentation), position_(p) {
    index = classes_.size();
    classes_.push_back(this);
}

bool Class::canBeUsedToResolve(TypeDefinitionWithGenerics *resolutionConstraint) {
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

Initializer* Class::getInitializer(const Token &token, Type type, TypeContext typeContext) {
    auto initializer = lookupInitializer(token.value[0]);
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], initializerString);
        throw CompilerErrorException(token, "%s has no initializer %s.", typeString.c_str(), initializerString);
    }
    return initializer;
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

Method* Class::getMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupMethod(token.value[0]);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], method);
        throw CompilerErrorException(token, "%s has no method %s", eclass.c_str(), method);
    }
    return method;
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

ClassMethod* Class::getClassMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupClassMethod(token.value[0]);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], method);
        throw CompilerErrorException(token, "%s has no class method %s", eclass.c_str(), method);
    }
    return method;
}

void Class::addClassMethod(ClassMethod *method) {
    duplicateDeclarationCheck(method, classMethods_, method->position());
    classMethods_[method->name] = method;
    classMethodList.push_back(method);
}

void Class::addMethod(Method *method) {
    duplicateDeclarationCheck(method, methods_, method->position());
    methods_[method->name] = method;
    methodList.push_back(method);
}

void Class::addInitializer(Initializer *init) {
    duplicateDeclarationCheck(init, initializers_, init->position());
    initializers_[init->name] = init;
    initializerList.push_back(init);
    
    if (init->required) {
        requiredInitializers_.insert(init->name);
    }
}

bool Class::addProtocol(Type protocol) {
    protocols_.push_back(protocol);
    return true;
}
