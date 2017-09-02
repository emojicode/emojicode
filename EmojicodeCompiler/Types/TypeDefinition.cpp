//
//  TypeDefinition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinition.hpp"
#include "../Analysis/BoxingLayerBuilder.hpp"
#include "../Application.hpp"
#include "../CompilerError.hpp"
#include "../Functions/BoxingLayer.hpp"
#include "../Functions/Function.hpp"
#include "../Functions/Initializer.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/Type.hpp"
#include "../Types/TypeContext.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

void TypeDefinition::setSuperType(const Type &type) {
    offsetIndicesBy(type.genericArguments().size());
    superType_ = type;
    for (size_t i = superType_.typeDefinition()->superGenericArguments().size(); i < superType_.genericArguments().size(); i++) {
        if (type.genericArguments()[i].type() == TypeType::GenericVariable) {
            auto newIndex = superType_.genericArguments()[i].genericVariableIndex() + superType_.genericArguments().size();
            superType_.setGenericArgument(i, Type(false, newIndex, this));
        }
    }
}

Initializer* TypeDefinition::lookupInitializer(const std::u32string &name) {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinition::lookupMethod(const std::u32string &name) {
    auto pos = methods_.find(name);
    if (pos != methods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Function* TypeDefinition::lookupTypeMethod(const std::u32string &name) {
    auto pos = typeMethods_.find(name);
    if (pos != typeMethods_.end()) {
        return pos->second;
    }
    return nullptr;
}

Initializer* TypeDefinition::getInitializer(const std::u32string &name, const Type &type,
                                                      const TypeContext &typeContext, const SourcePosition &p) {
    auto initializer = lookupInitializer(name);
    if (initializer == nullptr) {
        throw CompilerError(p, type.toString(typeContext), " has no initializer ", utf8(name), ".");
    }
    return initializer;
}

Function* TypeDefinition::getMethod(const std::u32string &name, const Type &type,
                                              const TypeContext &typeContext, const SourcePosition &p) {
    auto method = lookupMethod(name);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext);
        throw CompilerError(p, type.toString(typeContext), " has no method ", utf8(name), ".");
    }
    return method;
}

Function* TypeDefinition::getTypeMethod(const std::u32string &name, const Type &type,
                                                  const TypeContext &typeContext, const SourcePosition &p) {
    auto method = lookupTypeMethod(name);
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
        package_->app()->warn(position(), "Type defines ", instanceVariables_.size(),
                              " instances variables but has no initializers.");
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
                auto typeName = type.toString(TypeContext());
                auto protocolName = protocol.toString(TypeContext());
                throw CompilerError(position(), typeName, " does not agree to protocol ", protocolName ,": Method ",
                                    utf8(method->name()), "is missing.");
            }

            if (!clm->enforcePromises(method, TypeContext(type), protocol, TypeContext(protocol))) {
                auto arguments = std::vector<Argument>();
                arguments.reserve(method->arguments.size());
                for (auto &arg : method->arguments) {
                    arguments.emplace_back(arg.variableName, arg.type.resolveOn(TypeContext(protocol)));
                }
                auto bl = new BoxingLayer(clm, protocol.protocol()->name(), arguments,
                                          method->returnType.resolveOn(TypeContext(protocol)), clm->position());
                buildBoxingLayerAst(bl);
                if (enqueBoxingLayers) {
                    package_->app()->analysisQueue.emplace(bl);
                }
                method->registerOverrider(bl);
                addMethod(bl);
            }
            else {
                method->registerOverrider(clm);
            }
        }
        catch (CompilerError &ce) {
            package_->app()->error(ce);
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
