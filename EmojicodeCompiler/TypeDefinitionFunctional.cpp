//
//  TypeDefinitionFunctional.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinitionFunctional.hpp"
#include "CompilerErrorException.hpp"
#include "Token.hpp"
#include "Type.hpp"
#include "Function.hpp"
#include "TypeContext.hpp"

void TypeDefinitionFunctional::addGenericArgument(const Token &variableName, Type constraint) {
    genericArgumentConstraints_.push_back(constraint);
    
    Type referenceType = Type(TypeContent::Reference, false, ownGenericArgumentCount_, this);
    
    if (ownGenericArgumentVariables_.count(variableName.value())) {
        throw CompilerErrorException(variableName, "A generic argument variable with the same name is already in use.");
    }
    ownGenericArgumentVariables_.emplace(variableName.value(), referenceType);
    ownGenericArgumentCount_++;
}

void TypeDefinitionFunctional::setSuperTypeDef(TypeDefinitionFunctional *superTypeDef) {
    genericArgumentCount_ = ownGenericArgumentCount_ + superTypeDef->genericArgumentCount_;
    genericArgumentConstraints_.insert(genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.end());
    
    for (auto &genericArg : ownGenericArgumentVariables_) {
        genericArg.second.reference_ += superTypeDef->genericArgumentCount_;
    }
}

void TypeDefinitionFunctional::setSuperGenericArguments(std::vector<Type> superGenericArguments) {
    superGenericArguments_ = superGenericArguments;
}

void TypeDefinitionFunctional::finalizeGenericArguments() {
    genericArgumentCount_ = ownGenericArgumentCount_;
}

bool TypeDefinitionFunctional::fetchVariable(EmojicodeString name, bool optional, Type *destType) {
    auto it = ownGenericArgumentVariables_.find(name);
    if (it != ownGenericArgumentVariables_.end()) {
        Type type = it->second;
        if (optional) type.setOptional();
        *destType = type;
        return true;
    }
    return false;
}

Initializer* TypeDefinitionFunctional::lookupInitializer(EmojicodeString name) {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupMethod(EmojicodeString name) {
    auto pos = methods_.find(name);
    if (pos != methods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupTypeMethod(EmojicodeString name) {
    auto pos = typeMethods_.find(name);
    if (pos != typeMethods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Initializer* TypeDefinitionFunctional::getInitializer(const Token &token, Type type, TypeContext typeContext) {
    auto initializer = lookupInitializer(token.value());
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext, true);
        throw CompilerErrorException(token, "%s has no initializer %s.", typeString.c_str(),
                                     token.value().utf8().c_str());
    }
    return initializer;
}

Function* TypeDefinitionFunctional::getMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupMethod(token.value());
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        throw CompilerErrorException(token, "%s has no method %s", eclass.c_str(), token.value().utf8().c_str());
    }
    return method;
}

Function* TypeDefinitionFunctional::getTypeMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupTypeMethod(token.value());
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        throw CompilerErrorException(token, "%s has no type method %s", eclass.c_str(), token.value().utf8().c_str());
    }
    return method;
}

void TypeDefinitionFunctional::addTypeMethod(Function *method) {
    duplicateDeclarationCheck(method, typeMethods_, method->position());
    typeMethods_[method->name()] = method;
    typeMethodList_.push_back(method);
}

void TypeDefinitionFunctional::addMethod(Function *method) {
    duplicateDeclarationCheck(method, methods_, method->position());
    methods_[method->name()] = method;
    methodList_.push_back(method);
}

void TypeDefinitionFunctional::addInitializer(Initializer *init) {
    duplicateDeclarationCheck(init, initializers_, init->position());
    initializers_[init->name()] = init;
    initializerList_.push_back(init);

    if (init->required) {
        handleRequiredInitializer(init);
    }
}

void TypeDefinitionFunctional::addInstanceVariable(const InstanceVariableDeclaration &variable) {
    instanceVariables_.push_back(variable);
}

void TypeDefinitionFunctional::handleRequiredInitializer(Initializer *init) {
    throw CompilerErrorException(init->position(), "Required initializer not supported.");
}

void TypeDefinitionFunctional::finalize() {
    for (auto &var : instanceVariables()) {
        instanceScope().setLocalVariable(var.name, var.type, false, var.position);
    }
    
    if (instanceVariables().size() > 65536) {
        throw CompilerErrorException(position(), "You exceeded the limit of 65,536 instance variables.");
    }
    
    if (instanceVariables().size() > 0 && initializerList().size() == 0) {
        compilerWarning(position(), "Type defines %d instances variables but has no initializers.",
                        instanceVariables().size());
    }
}
