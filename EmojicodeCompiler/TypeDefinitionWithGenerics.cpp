//
//  TypeDefinitionWithGenerics.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinitionWithGenerics.hpp"
#include "Token.hpp"
#include "Type.hpp"

void TypeDefinitionWithGenerics::addGenericArgument(const Token &variableName, Type constraint) {
    genericArgumentConstraints_.push_back(constraint);
    
    Type referenceType = Type(TT_REFERENCE, false, ownGenericArgumentCount_, this);
    
    if (ownGenericArgumentVariables_.count(variableName.value)) {
        compilerError(variableName, "A generic argument variable with the same name is already in use.");
    }
    ownGenericArgumentVariables_.insert(std::map<EmojicodeString, Type>::value_type(variableName.value, referenceType));
    ownGenericArgumentCount_++;
}

void TypeDefinitionWithGenerics::setSuperTypeDef(TypeDefinitionWithGenerics *superTypeDef) {
    genericArgumentCount_ = ownGenericArgumentCount_ + superTypeDef->genericArgumentCount_;
    genericArgumentConstraints_.insert(genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.end());
    
    for (auto &genericArg : ownGenericArgumentVariables_) {
        genericArg.second.reference += superTypeDef->genericArgumentCount_;
    }
}

void TypeDefinitionWithGenerics::setSuperGenericArguments(std::vector<Type> superGenericArguments) {
    superGenericArguments_ = superGenericArguments;
}

void TypeDefinitionWithGenerics::finalizeGenericArguments() {
    genericArgumentCount_ = ownGenericArgumentCount_;
}

bool TypeDefinitionWithGenerics::fetchVariable(EmojicodeString name, bool optional, Type *destType) {
    auto it = ownGenericArgumentVariables_.find(name);
    if (it != ownGenericArgumentVariables_.end()) {
        Type type = it->second;
        if (optional) type.setOptional();
        *destType = type;
        return true;
    }
    return false;
}
