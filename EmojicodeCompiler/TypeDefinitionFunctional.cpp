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
    
    if (ownGenericArgumentVariables_.count(variableName.value)) {
        throw CompilerErrorException(variableName, "A generic argument variable with the same name is already in use.");
    }
    ownGenericArgumentVariables_.insert(std::map<EmojicodeString, Type>::value_type(variableName.value, referenceType));
    ownGenericArgumentCount_++;
}

void TypeDefinitionFunctional::setSuperTypeDef(TypeDefinitionFunctional *superTypeDef) {
    genericArgumentCount_ = ownGenericArgumentCount_ + superTypeDef->genericArgumentCount_;
    genericArgumentConstraints_.insert(genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.end());
    
    for (auto &genericArg : ownGenericArgumentVariables_) {
        genericArg.second.reference += superTypeDef->genericArgumentCount_;
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

Initializer* TypeDefinitionFunctional::lookupInitializer(EmojicodeChar name) {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupMethod(EmojicodeChar name) {
    auto pos = methods_.find(name);
    if (pos != methods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupClassMethod(EmojicodeChar name) {
    auto pos = classMethods_.find(name);
    if (pos != classMethods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Initializer* TypeDefinitionFunctional::getInitializer(const Token &token, Type type, TypeContext typeContext) {
    auto initializer = lookupInitializer(token.value[0]);
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], initializerString);
        throw CompilerErrorException(token, "%s has no initializer %s.", typeString.c_str(), initializerString);
    }
    return initializer;
}

Function* TypeDefinitionFunctional::getMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupMethod(token.value[0]);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], method);
        throw CompilerErrorException(token, "%s has no method %s", eclass.c_str(), method);
    }
    return method;
}

Function* TypeDefinitionFunctional::getClassMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupClassMethod(token.value[0]);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], method);
        throw CompilerErrorException(token, "%s has no class method %s", eclass.c_str(), method);
    }
    return method;
}

void TypeDefinitionFunctional::addClassMethod(Function *method) {
    duplicateDeclarationCheck(method, classMethods_, method->position());
    classMethods_[method->name()] = method;
    classMethodList_.push_back(method);
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

void TypeDefinitionFunctional::handleRequiredInitializer(Initializer *init) {
    throw CompilerErrorException(init->position(), "Required initializer not supported.");
}
