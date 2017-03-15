//
//  TypeDefinitionFunctional.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinitionFunctional.hpp"
#include "CompilerError.hpp"
#include "Token.hpp"
#include "Type.hpp"
#include "Function.hpp"
#include "TypeContext.hpp"
#include "Protocol.hpp"
#include "BoxingLayer.hpp"

void TypeDefinitionFunctional::addGenericArgument(const Token &variableName, const Type &constraint) {
    genericArgumentConstraints_.push_back(constraint);

    Type referenceType = Type(TypeContent::GenericVariable, false, ownGenericArgumentVariables_.size(), this);

    if (ownGenericArgumentVariables_.count(variableName.value()) > 0) {
        throw CompilerError(variableName.position(),
                            "A generic argument variable with the same name is already in use.");
    }
    ownGenericArgumentVariables_.emplace(variableName.value(), referenceType);
}

void TypeDefinitionFunctional::setSuperTypeDef(TypeDefinitionFunctional *superTypeDef) {
    genericArgumentCount_ = ownGenericArgumentVariables_.size() + superTypeDef->genericArgumentCount_;
    genericArgumentConstraints_.insert(genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.end());

    for (auto &genericArg : ownGenericArgumentVariables_) {
        genericArg.second.genericArgumentIndex_ += superTypeDef->genericArgumentCount_;
    }
}

void TypeDefinitionFunctional::setSuperGenericArguments(std::vector<Type> superGenericArguments) {
    superGenericArguments_ = std::move(superGenericArguments);
}

void TypeDefinitionFunctional::finalizeGenericArguments() {
    genericArgumentCount_ = ownGenericArgumentVariables_.size();
}

bool TypeDefinitionFunctional::fetchVariable(const EmojicodeString &name, bool optional, Type *destType) {
    auto it = ownGenericArgumentVariables_.find(name);
    if (it != ownGenericArgumentVariables_.end()) {
        Type type = it->second;
        if (optional) {
            type.setOptional();
        }
        *destType = type;
        return true;
    }
    return false;
}

Initializer* TypeDefinitionFunctional::lookupInitializer(const EmojicodeString &name) {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupMethod(const EmojicodeString &name) {
    auto pos = methods_.find(name);
    if (pos != methods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinitionFunctional::lookupTypeMethod(const EmojicodeString &name) {
    auto pos = typeMethods_.find(name);
    if (pos != typeMethods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Initializer* TypeDefinitionFunctional::getInitializer(const Token &token, const Type &type, const TypeContext &typeContext) {
    auto initializer = lookupInitializer(token.value());
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext, true);
        throw CompilerError(token.position(), "%s has no initializer %s.", typeString.c_str(),
                                     token.value().utf8().c_str());
    }
    return initializer;
}

Function* TypeDefinitionFunctional::getMethod(const Token &token, const Type &type, const TypeContext &typeContext) {
    auto method = lookupMethod(token.value());
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        throw CompilerError(token.position(), "%s has no method %s", eclass.c_str(), token.value().utf8().c_str());
    }
    return method;
}

Function* TypeDefinitionFunctional::getTypeMethod(const Token &token, const Type &type, const TypeContext &typeContext) {
    auto method = lookupTypeMethod(token.value());
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        throw CompilerError(token.position(), "%s has no type method %s", eclass.c_str(), token.value().utf8().c_str());
    }
    return method;
}

void TypeDefinitionFunctional::addProtocol(const Type &type, const SourcePosition &p) {
    for (auto &protocol : protocols_) {
        if (protocol.identicalTo(type, Type::nothingness(), nullptr)) {
            auto name = type.toString(Type::nothingness(), true);
            throw CompilerError(p, "Conformance to protocol %s was already declared.", name.c_str());
        }
    }
    protocols_.push_back(type);
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

void TypeDefinitionFunctional::addInitializer(Initializer *initializer) {
    duplicateDeclarationCheck(initializer, initializers_, initializer->position());
    initializers_[initializer->name()] = initializer;
    initializerList_.push_back(initializer);

    if (initializer->required()) {
        handleRequiredInitializer(initializer);
    }
}

void TypeDefinitionFunctional::addInstanceVariable(const InstanceVariableDeclaration &variable) {
    instanceVariables_.push_back(variable);
}

void TypeDefinitionFunctional::handleRequiredInitializer(Initializer *init) {
    throw CompilerError(init->position(), "Required initializer not supported.");
}

void TypeDefinitionFunctional::finalize() {
    for (auto &var : instanceVariables()) {
        instanceScope().setLocalVariable(var.name, var.type, false, var.position);
    }

    if (instanceVariables().size() > 65536) {
        throw CompilerError(position(), "You exceeded the limit of 65,536 instance variables.");
    }

    if (!instanceVariables().empty() && initializerList().empty()) {
        compilerWarning(position(), "Type defines %d instances variables but has no initializers.",
                        instanceVariables().size());
    }
}

void TypeDefinitionFunctional::finalizeProtocols(const Type &type, VTIProvider *methodVtiProvider) {
    for (const Type &protocol : protocols()) {
        for (auto method : protocol.protocol()->methodList()) {
            try {
                Function *clm = lookupMethod(method->name());
                if (clm == nullptr) {
                    auto typeName = type.toString(Type::nothingness(), true);
                    auto protocolName = protocol.toString(Type::nothingness(), true);
                    throw CompilerError(position(), "%s does not agree to protocol %s: Method %s is missing.",
                                        typeName.c_str(), protocolName.c_str(), method->name().utf8().c_str());
                }

                if (!clm->enforcePromises(method, type, protocol, TypeContext(protocol))) {
                    auto bl = new BoxingLayer(clm, protocol.protocol()->name(), methodVtiProvider,
                                              method->returnType.resolveOn(protocol), clm->position());
                    for (auto arg : method->arguments) {
                        bl->arguments.emplace_back(arg.variableName, arg.type.resolveOn(protocol));
                    }
                    method->registerOverrider(bl);
                    addMethod(bl);
                    bl->assignVti();
                }
                else {
                    method->registerOverrider(clm);
                    clm->assignVti();
                }
            }
            catch (CompilerError &ce) {
                printError(ce);
            }
        }
    }
}
