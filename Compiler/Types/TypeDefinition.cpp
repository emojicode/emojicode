//
//  TypeDefinition.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "TypeDefinition.hpp"
#include "CompilerError.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Types/Type.hpp"
#include <algorithm>

namespace EmojicodeCompiler {

TypeDefinition::TypeDefinition(std::u32string name, Package *p, SourcePosition pos, std::u32string documentation,
                               bool exported)
    : name_(std::move(name)), package_(p), documentation_(std::move(documentation)), position_(pos),
      exported_(exported) {}

TypeDefinition::~TypeDefinition() = default;

Function* TypeDefinition::deinitializer() {
    if (deinitializer_ == nullptr) {
        deinitializer_ = std::make_unique<Function>(U"deinit", AccessLevel::Public, false, this, package(),
                                                    this->position(), false, U"", false, false, Mood::Imperative, false,
                                                    FunctionType::Deinitializer);
        deinitializer_->setReturnType(std::make_unique<ASTLiteralType>(Type::noReturn()));
    }
    return deinitializer_.get();
}

Initializer* TypeDefinition::lookupInitializer(const std::u32string &name) const {
    auto pos = initializers_.find(name);
    if (pos != initializers_.end()) {
        return pos->second.get();
    }
    return nullptr;
}

Function* TypeDefinition::lookupMethod(const std::u32string &name, Mood mood) const {
    auto pos = methods_.find(methodTableName(name, mood));
    if (pos != methods_.end()) {
        return pos->second.get();
    }
    return nullptr;
}

Function* TypeDefinition::lookupTypeMethod(const std::u32string &name, Mood mood) const {
    auto pos = typeMethods_.find(methodTableName(name, mood));
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
                                    Mood mood, const SourcePosition &p) const {
    auto method = lookupMethod(name, mood);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext);
        throw CompilerError(p, type.toString(typeContext), " has no method ", utf8(name), ".");
    }
    return method;
}

Function * TypeDefinition::getTypeMethod(const std::u32string &name, const Type &type, const TypeContext &typeContext,
                                         Mood mood, const SourcePosition &p) const {
    auto method = lookupTypeMethod(name, mood);
    if (method == nullptr) {
        throw CompilerError(p, type.toString(typeContext), " has no type method ", utf8(name), ".");
    }
    return method;
}

Function* TypeDefinition::addTypeMethod(std::unique_ptr<Function> &&method) {
    duplicateDeclarationCheck(method.get(), typeMethods_);
    auto rawPtr = method.get();
    typeMethods_.emplace(methodTableName(rawPtr->name(), rawPtr->mood()), std::move(method));
    typeMethodList_.push_back(rawPtr);
    return rawPtr;
}

Function* TypeDefinition::addMethod(std::unique_ptr<Function> &&method) {
    duplicateDeclarationCheck(method.get(), methods_);
    auto rawPtr = method.get();
    methods_.emplace(methodTableName(rawPtr->name(), rawPtr->mood()), std::move(method));
    methodList_.push_back(rawPtr);
    return rawPtr;
}

std::u32string TypeDefinition::methodTableName(const std::u32string &name, Mood mood) const {
    switch (mood) {
    case Mood::Assignment:
        return name + U"=";
    case Mood::Interogative:
        return name + U"?";
    case Mood::Imperative:
        return name;
    }
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
    auto duplicate = std::find_if(instanceVariables_.begin(), instanceVariables_.end(),
                                  [&variable](auto &b) { return variable.name == b.name; });
    if (duplicate != instanceVariables_.end()) {
        throw CompilerError(variable.position, "Redeclaration of instance variable.");
    }
    instanceVariables_.push_back(variable);
}

void TypeDefinition::handleRequiredInitializer(Initializer *init) {

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
