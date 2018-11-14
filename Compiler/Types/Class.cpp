//
//  Class.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05.01.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Utils/StringUtils.hpp"
#include "Package/Package.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Class.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "Functions/Initializer.hpp"
#include "TypeContext.hpp"
#include <algorithm>
#include <utility>

namespace EmojicodeCompiler {

Class::Class(std::u32string name, Package *pkg, SourcePosition p, const std::u32string &documentation, bool exported,
             bool final, bool foreign) : TypeDefinition(std::move(name), pkg, p, documentation, exported),
                                         final_(final), foreign_(foreign) {
    instanceScope() = Scope(2);  // reassign a scoper with one offset for the pointer to the class meta
}

std::vector<Type> Class::superGenericArguments() const {
    if (superType_ != nullptr && superType_->wasAnalysed()) {
        return superType_->type().genericArguments();
    }
    return std::vector<Type>();
}

void Class::analyseSuperType() {
    if (superType() == nullptr) {
        return;
    }

    auto classType = Type(this);
    auto &type = superType()->analyseType(TypeContext(classType));

    if (type.type() != TypeType::Class) {
        throw CompilerError(superType()->position(), "The superclass must be a class.");
    }

    if (type.klass()->superType() != nullptr && !type.klass()->superType()->wasAnalysed()) {
        type.klass()->analyseSuperType();
    }

    if (type.klass()->final()) {
        package()->compiler()->error(CompilerError(superType()->position(), type.toString(TypeContext(classType)),
                                                  " can’t be used as superclass as it was marked with 🔏."));
    }
    type.klass()->setHasSubclass();

    std::vector<std::u32string> missing;
    std::set_difference(type.klass()->requiredInitializers_.begin(), type.klass()->requiredInitializers_.end(),
                        requiredInitializers_.begin(), requiredInitializers_.end(), std::back_inserter(missing));
    for (auto &name : missing) {
        package()->compiler()->error(CompilerError(position(), "Required initializer ",
                                                   utf8(name), " was not implemented."));
    }

    offsetIndicesBy(type.genericArguments().size());
    for (size_t i = type.typeDefinition()->superGenericArguments().size(); i < type.genericArguments().size(); i++) {
        if (type.genericArguments()[i].type() == TypeType::GenericVariable) {
            auto newIndex = type.genericArguments()[i].genericVariableIndex() + type.genericArguments().size();
            type.setGenericArgument(i, Type(newIndex, this));
        }
        else if (type.genericArguments()[i].type() == TypeType::Box &&
                 type.genericArguments()[i].unboxedType() == TypeType::GenericVariable) {
            auto newIndex = type.genericArguments()[i].unboxed().genericVariableIndex() + type.genericArguments().size();
            type.setGenericArgument(i, Type(newIndex, this).boxedFor(type.genericArguments()[i].boxedFor()));
        }
    }
}

void Class::inherit(SemanticAnalyser *analyser) {
    if (superType() == nullptr) {
        analyser->declareInstanceVariables(Type(this));
        eachFunction([](auto *function) {
            if (function->accessLevel() == AccessLevel::Default) {
                function->setAccessLevel(AccessLevel::Public);
            }
        });
        return;
    }

    if (instanceVariables().empty() && initializerList().empty()) {
        inheritsInitializers_ = true;
    }

    instanceScope() = superclass()->instanceScope();
    instanceScope().markInherited();

    analyser->declareInstanceVariables(Type(this));

    Type classType = Type(this);
    instanceVariablesMut().insert(instanceVariables().begin(), superclass()->instanceVariables().begin(),
                                  superclass()->instanceVariables().end());

    protocols_.reserve(superclass()->protocols().size());
    for (auto &protocol : superclass()->protocols()) {
        auto find = std::find_if(protocols_.begin(), protocols_.end(), [&classType, &protocol](auto &a) {
            return a->type().identicalTo(protocol->type(), TypeContext(classType), nullptr);
        });
        if (find != protocols_.end()) {
            analyser->compiler()->error(CompilerError(position(), "Superclass already declared conformance to ",
                                                      protocol->type().toString(TypeContext(classType)), "."));
        }
        protocols_.emplace_back(protocol);
    }

    eachFunctionWithoutInitializers([this, analyser](Function *function) {
        checkOverride(function, analyser);
    });
}

void Class::checkOverride(Function *function, SemanticAnalyser *analyser) {
    auto superFunction = findSuperFunction(function);
    if (function->overriding()) {
        if (superFunction == nullptr) {
            throw CompilerError(function->position(), utf8(function->name()),
                                " was declared ✒️ but does not override anything.");
        }
        auto layer = analyser->enforcePromises(function, superFunction, Type(superclass()),
                                               TypeContext(Type(this)), TypeContext());
        if (function->accessLevel() == AccessLevel::Default) {
            function->setAccessLevel(superFunction->accessLevel());
        }
        if (layer != nullptr) {
            function->setVirtualTableThunk(layer.get());
            addMethod(std::move(layer));
        }
    }
    else if (superFunction != nullptr) {
        analyser->compiler()->error(CompilerError(function->position(), "If you want to override ",
                                                  utf8(function->name()), " add ✒️."));
    }
    else if (function->accessLevel() == AccessLevel::Default) {
        function->setAccessLevel(AccessLevel::Public);
    }
}

void Class::addInstanceVariable(const InstanceVariableDeclaration &declaration) {
    if (foreign()) {
        throw CompilerError(position(), "Instance variables are not allowed in foreign class.");
    }
    TypeDefinition::addInstanceVariable(declaration);
}

bool Class::canResolve(TypeDefinition *resolutionConstraint) const {
    if (auto cl = dynamic_cast<Class *>(resolutionConstraint)) {
        return inheritsFrom(cl);
    }
    return false;
}

bool Class::inheritsFrom(Class *from) const {
    for (const Class *a = this; a != nullptr; a = a->superclass()) {
        if (a == from) {
            return true;
        }
    }
    return false;
}

Initializer* Class::lookupInitializer(const std::u32string &name) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto initializer = klass->TypeDefinition::lookupInitializer(name)) {
            return initializer;
        }
        if (!klass->inheritsInitializers()) {
            break;
        }
    }
    return nullptr;
}

Function * Class::lookupMethod(const std::u32string &name, bool imperative) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupMethod(name, imperative)) {
            return method;
        }
    }
    return nullptr;
}

Function * Class::lookupTypeMethod(const std::u32string &name, bool imperative) const {
    for (auto klass = this; klass != nullptr; klass = klass->superclass()) {
        if (auto method = klass->TypeDefinition::lookupTypeMethod(name, imperative)) {
            return method;
        }
    }
    return nullptr;
}

void Class::handleRequiredInitializer(Initializer *init) {
    requiredInitializers_.insert(init->name());
}


}  // namespace EmojicodeCompiler
