//
//  TypeDefinition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinition.hpp"
#include "Analysis/BoxingLayerBuilder.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "Functions/BoxingLayer.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Types/TypeContext.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

TypeDefinition::TypeDefinition(std::u32string name, Package *p, SourcePosition pos, std::u32string documentation,
                               bool exported)
    : name_(std::move(name)), package_(p), documentation_(std::move(documentation)), position_(std::move(pos)),
      exported_(exported) {}

TypeDefinition::~TypeDefinition() = default;

void TypeDefinition::setSuperType(const Type &type) {
    offsetIndicesBy(type.genericArguments().size());
    superType_ = type;
    for (size_t i = superType_.typeDefinition()->superGenericArguments().size(); i < superType_.genericArguments().size(); i++) {
        if (type.genericArguments()[i].type() == TypeType::GenericVariable) {
            auto newIndex = superType_.genericArguments()[i].genericVariableIndex() + superType_.genericArguments().size();
            superType_.setGenericArgument(i, Type(false, newIndex, this, true));
        }
    }
}

Initializer* TypeDefinition::lookupInitializer(const std::u32string &name) const {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second.get();
    }
    return nullptr;
}

Function* TypeDefinition::lookupMethod(const std::u32string &name, bool imperative) const {
    auto pos = methods_.find(methodTableName(name, imperative));
    if (pos != methods_.end()) {
        return pos->second.get();
    }
    return nullptr;
}

Function* TypeDefinition::lookupTypeMethod(const std::u32string &name, bool imperative) const {
    auto pos = typeMethods_.find(methodTableName(name, imperative));
    if (pos != typeMethods_.end()) {
        return pos->second.get();
    }
    return nullptr;
}

Initializer* TypeDefinition::getInitializer(const std::u32string &name, const Type &type,
                                            const TypeContext &typeContext, const SourcePosition &p) const {
    auto initializer = lookupInitializer(name);
    if (initializer == nullptr) {
        throw CompilerError(p, type.toString(typeContext), " has no initializer ", utf8(name), ".");
    }
    return initializer;
}

Function* TypeDefinition::getMethod(const std::u32string &name, const Type &type, const TypeContext &typeContext,
                                    bool imperative, const SourcePosition &p) const {
    auto method = lookupMethod(name, imperative);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext);
        throw CompilerError(p, type.toString(typeContext), " has no method ", utf8(name), ".");
    }
    return method;
}

Function * TypeDefinition::getTypeMethod(const std::u32string &name, const Type &type, const TypeContext &typeContext,
                                         bool imperative, const SourcePosition &p) const {
    auto method = lookupTypeMethod(name, imperative);
    if (method == nullptr) {
        throw CompilerError(p, type.toString(typeContext), " has no type method ", utf8(name), ".");
    }
    return method;
}

void TypeDefinition::addProtocol(const Type &type, const SourcePosition &p) {
    for (auto &protocol : protocols_) {
        if (protocol.identicalTo(type, TypeContext(), nullptr)) {
            auto name = type.toString(TypeContext());
            throw CompilerError(p, "Conformance to protocol ", name, " was already declared.");
        }
    }
    protocols_.push_back(type);
}

Function* TypeDefinition::addTypeMethod(std::unique_ptr<Function> &&method) {
    duplicateDeclarationCheck(method.get(), typeMethods_);
    auto rawPtr = method.get();
    typeMethods_.emplace(methodTableName(rawPtr->name(), rawPtr->isImperative()), std::move(method));
    typeMethodList_.push_back(rawPtr);
    return rawPtr;
}

Function* TypeDefinition::addMethod(std::unique_ptr<Function> &&method) {
    duplicateDeclarationCheck(method.get(), methods_);
    auto rawPtr = method.get();
    methods_.emplace(methodTableName(rawPtr->name(), rawPtr->isImperative()), std::move(method));
    methodList_.push_back(rawPtr);
    return rawPtr;
}

std::u32string TypeDefinition::methodTableName(const std::u32string &name, bool imperative) const {
    return imperative ? name : name + std::u32string(1, '?');
}

Initializer* TypeDefinition::addInitializer(std::unique_ptr<Initializer> &&initializer) {
    duplicateDeclarationCheck(initializer.get(), initializers_);
    auto rawPtr = initializer.get();
    initializers_.emplace(initializer->name(), std::move(initializer));
    initializerList_.push_back(rawPtr);

    if (rawPtr->required()) {
        handleRequiredInitializer(rawPtr);
    }
    return rawPtr;
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
        package_->compiler()->warn(position(), "Type defines ", instanceVariables_.size(),
                              " instances variables but has no initializers.");
    }
}

void TypeDefinition::finalizeProtocol(const Type &type, const Type &protocol, bool enqueBoxingLayers) {
    for (auto method : protocol.protocol()->methodList()) {
        try {
            Function *clm = lookupMethod(method->name(), method->isImperative());
            if (clm == nullptr) {
                auto typeName = type.toString(TypeContext());
                auto protocolName = protocol.toString(TypeContext());
                throw CompilerError(position(), typeName, " does not agree to protocol ", protocolName ,": Method ",
                                    utf8(method->name()), "is missing.");
            }

            clm->createUnspecificReification();
            if (!clm->enforcePromises(method, TypeContext(type), protocol, TypeContext(protocol))) {
                auto arguments = std::vector<Argument>();
                arguments.reserve(method->arguments().size());
                for (auto &arg : method->arguments()) {
                    arguments.emplace_back(arg.variableName, arg.type.resolveOn(TypeContext(protocol)));
                }
                auto bl = std::make_unique<BoxingLayer>(clm, protocol.protocol()->name(), arguments,
                                                        method->returnType().resolveOn(TypeContext(protocol)),
                                                        clm->position());
                buildBoxingLayerAst(bl.get());
                if (enqueBoxingLayers) {
                    package_->compiler()->analysisQueue.emplace(bl.get());
                }
                method->appointHeir(bl.get());
                addMethod(std::move(bl));
            }
            else {
                method->appointHeir(clm);
            }
        }
        catch (CompilerError &ce) {
            package_->compiler()->error(ce);
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
