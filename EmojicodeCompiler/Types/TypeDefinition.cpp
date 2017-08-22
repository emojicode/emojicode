//
//  TypeDefinition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinition.hpp"
#include "../Analysis/BoxingLayerBuilder.hpp"
#include "../BoxingLayer.hpp"
#include "../CompilerError.hpp"
#include "../Function.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/Type.hpp"
#include "../Types/TypeContext.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

void TypeDefinition::addGenericArgument(const EmojicodeString &variableName, const Type &constraint,
                                        const SourcePosition &p) {
    genericArgumentConstraints_.push_back(constraint);

    Type referenceType = Type(TypeContent::GenericVariable, false, ownGenericArgumentVariables_.size(), this);

    if (ownGenericArgumentVariables_.count(variableName) > 0) {
        throw CompilerError(p, "A generic argument variable with the same name is already in use.");
    }
    ownGenericArgumentVariables_.emplace(variableName, referenceType);
}

void TypeDefinition::setSuperTypeDef(TypeDefinition *superTypeDef) {
    genericArgumentCount_ = ownGenericArgumentVariables_.size() + superTypeDef->genericArgumentCount_;
    genericArgumentConstraints_.insert(genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.begin(),
                                       superTypeDef->genericArgumentConstraints_.end());

    for (auto &genericArg : ownGenericArgumentVariables_) {
        genericArg.second.genericArgumentIndex_ += superTypeDef->genericArgumentCount_;
    }
}

void TypeDefinition::setSuperGenericArguments(std::vector<Type> superGenericArguments) {
    superGenericArguments_ = std::move(superGenericArguments);
}

void TypeDefinition::finalizeGenericArguments() {
    genericArgumentCount_ = ownGenericArgumentVariables_.size();
}

bool TypeDefinition::fetchVariable(const EmojicodeString &name, bool optional, Type *destType) {
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

Initializer* TypeDefinition::lookupInitializer(const EmojicodeString &name) {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinition::lookupMethod(const EmojicodeString &name) {
    auto pos = methods_.find(name);
    if (pos != methods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinition::lookupTypeMethod(const EmojicodeString &name) {
    auto pos = typeMethods_.find(name);
    if (pos != typeMethods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Initializer* TypeDefinition::getInitializer(const EmojicodeString &name, const Type &type,
                                                      const TypeContext &typeContext, const SourcePosition &p) {
    auto initializer = lookupInitializer(name);
    if (initializer == nullptr) {
        auto typeString = type.toString(typeContext);
        throw CompilerError(p, "%s has no initializer %s.", typeString.c_str(), name.utf8().c_str());
    }
    return initializer;
}

Function* TypeDefinition::getMethod(const EmojicodeString &name, const Type &type,
                                              const TypeContext &typeContext, const SourcePosition &p) {
    auto method = lookupMethod(name);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext);
        throw CompilerError(p, "%s has no method %s", eclass.c_str(), name.utf8().c_str());
    }
    return method;
}

Function* TypeDefinition::getTypeMethod(const EmojicodeString &name, const Type &type,
                                                  const TypeContext &typeContext, const SourcePosition &p) {
    auto method = lookupTypeMethod(name);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext);
        throw CompilerError(p, "%s has no type method %s", eclass.c_str(), name.utf8().c_str());
    }
    return method;
}

void TypeDefinition::addProtocol(const Type &type, const SourcePosition &p) {
    for (auto &protocol : protocols_) {
        if (protocol.identicalTo(type, Type::nothingness(), nullptr)) {
            auto name = type.toString(Type::nothingness());
            throw CompilerError(p, "Conformance to protocol %s was already declared.", name.c_str());
        }
    }
    protocols_.push_back(type);
}

void TypeDefinition::addTypeMethod(Function *method) {
    nativeCheck(method);
    duplicateDeclarationCheck(method, typeMethods_, method->position());
    typeMethods_[method->name()] = method;
    typeMethodList_.push_back(method);
}

void TypeDefinition::addMethod(Function *method) {
    nativeCheck(method);
    duplicateDeclarationCheck(method, methods_, method->position());
    methods_[method->name()] = method;
    methodList_.push_back(method);
}

void TypeDefinition::addInitializer(Initializer *initializer) {
    nativeCheck(initializer);
    duplicateDeclarationCheck(initializer, initializers_, initializer->position());
    initializers_[initializer->name()] = initializer;
    initializerList_.push_back(initializer);

    if (initializer->required()) {
        handleRequiredInitializer(initializer);
    }
}

void TypeDefinition::nativeCheck(Function *function) {
    if (!function->isNative()) {
        return;
    }
    if (!package_->requiresBinary()) {
        throw CompilerError(position(), "Function was declared to have a native counterpart but package "\
                            "does not have a native binary.");
    }
    if (function->package() != package_) {
        throw CompilerError(position(), "Functions declared in another package than their owning type cannot have a "\
                            "native counterpart");
    }
}

void TypeDefinition::addInstanceVariable(const InstanceVariableDeclaration &variable) {
    instanceVariables_.push_back(variable);
}

void TypeDefinition::handleRequiredInitializer(Initializer *init) {
    throw CompilerError(init->position(), "Required initializer not supported.");
}

void TypeDefinition::prepareForSemanticAnalysis() {
    for (auto &var : instanceVariables_) {
        instanceScope().declareVariable(var.name, var.type, false, var.position);
    }

    if (instanceVariables_.size() > 65536) {
        throw CompilerError(position(), "You exceeded the limit of 65,536 instance variables.");
    }

    if (!instanceVariables_.empty() && initializerList_.empty()) {
        compilerWarning(position(), "Type defines %d instances variables but has no initializers.",
                        instanceVariables_.size());
    }
}

void TypeDefinition::prepareForCG() {
    createCGScope();

    for (auto &ivar : instanceVariables()) {
        auto &var = scope_.getLocalVariable(ivar.name);
        cgScoper_.declareVariable(var.id(), ivar.type).initialize(0);
    }
}

void TypeDefinition::finalizeProtocol(const Type &type, const Type &protocol, bool enqueBoxingLayers) {
    for (auto method : protocol.protocol()->methodList()) {
        try {
            Function *clm = lookupMethod(method->name());
            if (clm == nullptr) {
                auto typeName = type.toString(Type::nothingness());
                auto protocolName = protocol.toString(Type::nothingness());
                throw CompilerError(position(), "%s does not agree to protocol %s: Method %s is missing.",
                                    typeName.c_str(), protocolName.c_str(), method->name().utf8().c_str());
            }

            if (!clm->enforcePromises(method, type, protocol, TypeContext(protocol))) {
                auto arguments = std::vector<Argument>();
                arguments.reserve(method->arguments.size());
                for (auto &arg : method->arguments) {
                    arguments.emplace_back(arg.variableName, arg.type.resolveOn(protocol));
                }
                auto bl = new BoxingLayer(clm, protocol.protocol()->name(), arguments,
                                          method->returnType.resolveOn(protocol), clm->position());
                buildBoxingLayerAst(bl);
                if (enqueBoxingLayers) {
                    Function::analysisQueue.emplace(bl);
                }
                method->registerOverrider(bl);
                addMethod(bl);
            }
            else {
                method->registerOverrider(clm);
            }
        }
        catch (CompilerError &ce) {
            printError(ce);
        }
    }
}

void TypeDefinition::finalizeProtocols(const Type &type) {
    for (const Type &protocol : protocols()) {
        finalizeProtocol(type, protocol, true);
    }
}

void TypeDefinition::eachFunction(const std::function<void (Function *)>& cb) const {
    eachFunctionWithoutInitializers(cb);
    for (auto function : initializerList()) {
        cb(function);
    }
}

void TypeDefinition::eachFunctionWithoutInitializers(const std::function<void (Function *)>& cb) const {
    for (auto function : methodList()) {
        cb(function);
    }
    for (auto function : typeMethodList()) {
        cb(function);
    }
}

}  // namespace EmojicodeCompiler
