//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.hpp"
#include "Class.hpp"
#include "Procedure.hpp"
#include "utf8.h"
#include "Lexer.hpp"

bool Class::conformsTo(Protocol *to) {
    for(Class *a = this; a != nullptr; a = a->superclass) {
        for(size_t i = 0; i < this->protocols_.size(); i++) {
            if(a->protocols_[i] == to) {
                return true;
            }
        }
    }
    return false;
}

bool Class::inheritsFrom(Class *from) {
    for(Class *a = this; a != nullptr; a = a->superclass) {
        if(a == from) {
            return true;
        }
    }
    return false;
}

Initializer* Class::lookupInitializer(EmojicodeChar name) {
    for(auto eclass = this; eclass != nullptr; eclass = eclass->superclass) {
        auto pos = eclass->initializers.find(name);
        if(pos != eclass->initializers.end()) {
            return pos->second;
        }
        if(!eclass->inheritsContructors) { //Does this eclass inherit initializers?
            break;
        }
    }
    return nullptr;
}

Initializer* Class::getInitializer(const Token *token, Type type, TypeContext typeContext) {
    auto initializer = lookupInitializer(token->value[0]);
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext, true);
        ecCharToCharStack(token->value[0], initializerString);
        compilerError(token, "%s has no initializer %s.", typeString.c_str(), initializerString);
    }
    return initializer;
}

Method* Class::lookupMethod(EmojicodeChar name){
    for(auto eclass = this; eclass != nullptr; eclass = eclass->superclass){
        auto pos = eclass->methods.find(name);
        if(pos != eclass->methods.end()){
            return pos->second;
        }
    }
    return nullptr;
}

Method* Class::getMethod(const Token *token, Type type, TypeContext typeContext) {
    auto method = lookupMethod(token->value[0]);
    if (method == nullptr){
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token->value[0], method);
        compilerError(token, "%s has no method %s", eclass.c_str(), method);
    }
    return method;
}

ClassMethod* Class::lookupClassMethod(EmojicodeChar name){
    for(auto eclass = this; eclass != nullptr; eclass = eclass->superclass){
        auto pos = eclass->classMethods.find(name);
        if(pos != eclass->classMethods.end()){
            return pos->second;
        }
    }
    return nullptr;
}

ClassMethod* Class::getClassMethod(const Token *token, Type type, TypeContext typeContext) {
    auto method = lookupClassMethod(token->value[0]);
    if (method == nullptr){
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token->value[0], method);
        compilerError(token, "%s has no class method %s", eclass.c_str(), method);
    }
    return method;
}

template <typename T>
void duplicateDeclarationCheck(T p, std::map<EmojicodeChar, T> dict, const Token *errorToken) {
    if (dict.count(p->name)) {
        ecCharToCharStack(p->name, nameString);
        compilerError(errorToken, "%s %s is declared twice.", p->on, nameString);
    }
}

void Class::addClassMethod(ClassMethod *method){
    duplicateDeclarationCheck(method, classMethods, method->dToken);
    classMethods[method->name] = method;
    classMethodList.push_back(method);
}

void Class::addMethod(Method *method){
    duplicateDeclarationCheck(method, methods, method->dToken);
    methods[method->name] = method;
    methodList.push_back(method);
}

void Class::addInitializer(Initializer *init){
    duplicateDeclarationCheck(init, initializers, init->dToken);
    initializers[init->name] = init;
    initializerList.push_back(init);
}

void Class::addProtocol(Protocol *protocol){
    protocols_.push_back(protocol);
}

//MARK: Protocol

Method* Protocol::getMethod(EmojicodeChar name){
    auto it = methods_.find(name);
    return it != methods_.end() ? it->second : nullptr;
}

void Protocol::addMethod(Method *method){
    duplicateDeclarationCheck(method, methods_, method->dToken);
    method->vti = methodList_.size();
    methods_[method->name] = method;
    methodList_.push_back(method);
}

//MARK: Enum

std::pair<bool, EmojicodeInteger> Enum::getValueFor(EmojicodeChar c) const {
    auto it = map.find(c);
    if (it == map.end()) {
        return std::pair<bool, EmojicodeInteger>(false, 0);
    }
    else {
        return std::pair<bool, EmojicodeInteger>(true, it->second);
    }
}

void Enum::addValueFor(EmojicodeChar c){
    map[c] = valuesCounter++;
}