//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Type.hpp"
#include "Class.hpp"
#include "CommonTypeFinder.hpp"
#include "Emojis.h"
#include "Enum.hpp"
#include "Extension.hpp"
#include "Functions/Function.hpp"
#include "Package/Package.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"
#include "Utils/StringUtils.hpp"
#include "ValueType.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

const std::u32string kDefaultNamespace = std::u32string(1, E_HOUSE_BUILDING);

Type::Type(Protocol *protocol)
    : typeContent_(TypeType::Protocol), typeDefinition_(protocol) {}

Type::Type(Enum *enumeration)
    : typeContent_(TypeType::Enum), typeDefinition_(enumeration) {}

Type::Type(Extension *extension)
    : typeContent_(TypeType::Extension), typeDefinition_(extension) {}

Type::Type(ValueType *valueType)
: typeContent_(TypeType::ValueType), typeDefinition_(valueType), mutable_(false) {
    for (size_t i = 0; i < valueType->genericParameters().size(); i++) {
        genericArguments_.emplace_back(i, valueType, true);
    }
}

Type::Type(Class *klass) : typeContent_(TypeType::Class), typeDefinition_(klass) {
    for (size_t i = 0; i < klass->genericParameters().size() + klass->superGenericArguments().size(); i++) {
        genericArguments_.emplace_back(i, klass, true);
    }
}

Type::Type(Function *function) : typeContent_(TypeType::Callable) {
    genericArguments_.reserve(function->parameters().size() + 1);
    genericArguments_.emplace_back(function->returnType());
    for (auto &argument : function->parameters()) {
        genericArguments_.emplace_back(argument.type);
    }
}

Class* Type::klass() const {
    return dynamic_cast<Class *>(typeDefinition_);
}

Protocol* Type::protocol() const {
    return dynamic_cast<Protocol *>(typeDefinition_);
}

Enum* Type::enumeration() const {
    return dynamic_cast<Enum *>(typeDefinition_);
}

ValueType* Type::valueType() const {
    return dynamic_cast<ValueType *>(typeDefinition_);
}

TypeDefinition* Type::typeDefinition() const {
    assert(type() == TypeType::Class || type() == TypeType::Protocol || type() == TypeType::ValueType
           || type() == TypeType::Enum || type() == TypeType::Extension);
    return typeDefinition_;
}

bool Type::isExact() const {
    return forceExact_ || (type() == TypeType::Class && klass()->final());
}

bool Type::canHaveGenericArguments() const {
    return type() == TypeType::Class || type() == TypeType::Protocol || type() == TypeType::ValueType;
}

void Type::sortMultiProtocolType() {
    assert(type() == TypeType::MultiProtocol);
    std::sort(genericArguments_.begin(), genericArguments_.end(), [](const Type &a, const Type &b) {
        return a.protocol() < b.protocol();
    });
}

size_t Type::genericVariableIndex() const {
    if (type() != TypeType::GenericVariable && type() != TypeType::LocalGenericVariable) {
        throw std::domain_error("Tried to get reference from non-reference type");
    }
    return genericArgumentIndex_;
}

Type Type::resolveReferenceToBaseReferenceOnSuperArguments(const TypeContext &typeContext) const {
    TypeDefinition *c = typeContext.calleeType().typeDefinition();
    Type t = *this;

    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeType::GenericVariable && c->canBeUsedToResolve(t.typeDefinition_) &&
           t.genericVariableIndex() < c->superGenericArguments().size()) {
        Type tn = c->superGenericArguments()[t.genericVariableIndex()];
        if (tn.type() == TypeType::GenericVariable && tn.genericVariableIndex() == t.genericVariableIndex()
            && tn.typeDefinition_ == t.typeDefinition_) {
            break;
        }
        t = tn;
    }
    return t;
}

Type Type::resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext) const {
    Type t = *this;
    if (type() == TypeType::Optional) {
        t.genericArguments_[0] = genericArguments_[0].resolveOnSuperArgumentsAndConstraints(typeContext);
        return t;
    }

    TypeDefinition *c = nullptr;
    if (typeContext.calleeType().canHaveGenericArguments()) {
        c = typeContext.calleeType().typeDefinition();
    }
    else if (typeContext.calleeType().type() == TypeType::TypeAsValue) {
        c = typeContext.calleeType().typeOfTypeValue().typeDefinition();
    }
    bool box = t.storageType() == StorageType::Box;

    if (c != nullptr) {
        // Try to resolve on the generic arguments to the superclass.
        while (t.type() == TypeType::GenericVariable && t.genericVariableIndex() < c->superGenericArguments().size()) {
            t = c->superGenericArguments()[t.genericArgumentIndex_];
        }
    }
    while (t.type() == TypeType::LocalGenericVariable && typeContext.function() == t.localResolutionConstraint_) {
        t = typeContext.function()->constraintForIndex(t.genericArgumentIndex_);
    }
    if (c != nullptr) {
        while (t.type() == TypeType::GenericVariable && c->canBeUsedToResolve(t.typeDefinition_)) {
            t = c->constraintForIndex(t.genericArgumentIndex_);
        }
    }

    if (box) {
        t.forceBox_ = true;
    }
    return t;
}

Type Type::resolveOn(const TypeContext &typeContext) const {
    Type t = *this;
    if (type() == TypeType::Optional) {
        t.genericArguments_[0] = genericArguments()[0].resolveOn(typeContext);
        return t;
    }

    bool box = t.storageType() == StorageType::Box;

    while (t.type() == TypeType::LocalGenericVariable && typeContext.function() == t.localResolutionConstraint_
           && typeContext.functionGenericArguments() != nullptr) {
        t = (*typeContext.functionGenericArguments())[t.genericVariableIndex()];
    }

    if (typeContext.calleeType().canHaveGenericArguments()) {
        while (t.type() == TypeType::GenericVariable &&
               typeContext.calleeType().typeDefinition()->canBeUsedToResolve(t.typeDefinition_)) {
            Type tn = typeContext.calleeType().genericArguments_[t.genericVariableIndex()];
            if (tn.type() == TypeType::GenericVariable && tn.genericVariableIndex() == t.genericVariableIndex()
                && tn.typeDefinition_ == t.typeDefinition_) {
                break;
            }
            t = tn;
        }
    }

    for (auto &arg : t.genericArguments_) {
        arg = arg.resolveOn(typeContext);
    }

    if (box) {
        t.forceBox_ = true;
    }
    return t;
}

bool Type::identicalGenericArguments(Type to, const TypeContext &typeContext, std::vector<CommonTypeFinder> *ctargs) const {
    for (size_t i = to.typeDefinition()->superGenericArguments().size(); i < to.genericArguments().size(); i++) {
        if (!this->genericArguments_[i].identicalTo(to.genericArguments_[i], typeContext, ctargs)) {
            return false;
        }
    }
    return true;
}

bool Type::compatibleTo(const Type &to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeType::Error) {
        return identicalTo(to, tc, ctargs);
    }
    if (to.type() == TypeType::Something) {
        return true;
    }

    if (this->type() == TypeType::Optional) {
        if (to.type() != TypeType::Optional) {
            return false;
        }
        return optionalType().compatibleTo(to.optionalType(), tc, ctargs);
    }
    if (this->type() != TypeType::Optional && to.type() == TypeType::Optional) {
        return compatibleTo(to.optionalType(), tc, ctargs);
    }

    if ((this->type() == TypeType::GenericVariable && to.type() == TypeType::GenericVariable) ||
        (this->type() == TypeType::LocalGenericVariable && to.type() == TypeType::LocalGenericVariable)) {
        return (this->genericVariableIndex() == to.genericVariableIndex() &&
                this->typeDefinition_ == to.typeDefinition_) ||
        this->resolveOnSuperArgumentsAndConstraints(tc)
        .compatibleTo(to.resolveOnSuperArgumentsAndConstraints(tc), tc, ctargs);
    }
    if (type() == TypeType::GenericVariable) {
        return resolveOnSuperArgumentsAndConstraints(tc).compatibleTo(to, tc, ctargs);
    }
    if (type() == TypeType::LocalGenericVariable) {
        return ctargs != nullptr || resolveOnSuperArgumentsAndConstraints(tc).compatibleTo(to, tc, ctargs);
    }

    switch (to.type()) {
        case TypeType::TypeAsValue:
            return isCompatibleToTypeAsValue(to, tc, ctargs);
        case TypeType::GenericVariable:
            return compatibleTo(to.resolveOnSuperArgumentsAndConstraints(tc), tc, ctargs);
        case TypeType::LocalGenericVariable:
            if (ctargs != nullptr) {
                (*ctargs)[to.genericVariableIndex()].addType(*this, tc);
                return true;
            }
            return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(tc), tc, ctargs);
        case TypeType::Class:
            return type() == TypeType::Class && klass()->inheritsFrom(to.klass()) &&
                identicalGenericArguments(to, tc, ctargs);
        case TypeType::ValueType:
            return type() == TypeType::ValueType && typeDefinition() == to.typeDefinition() &&
                identicalGenericArguments(to, tc, ctargs);
        case TypeType::Enum:
            return type() == TypeType::Enum && enumeration() == to.enumeration();
        case TypeType::Someobject:
            return type() == TypeType::Class || type() == TypeType::Someobject;
        case TypeType::Error:
            return compatibleTo(to.errorType(), tc);
        case TypeType::MultiProtocol:
            return isCompatibleToMultiProtocol(to, tc, ctargs);
        case TypeType::Protocol:
            return isCompatibleToProtocol(to, tc, ctargs);
        case TypeType::Callable:
            return isCompatibleToCallable(to, tc, ctargs);
        case TypeType::NoReturn:
            return type() == TypeType::NoReturn;
        default:
            break;
    }
    return false;
}

bool Type::isCompatibleToTypeAsValue(const Type &to, const TypeContext &tc,
                                     std::vector<CommonTypeFinder> *ctargs) const {
    if (type() != TypeType::TypeAsValue) {
        return false;
    }

    if (typeOfTypeValue().type() == TypeType::Class && to.typeOfTypeValue().type() == TypeType::Class) {
        return typeOfTypeValue().compatibleTo(to.typeOfTypeValue(), tc, ctargs);
    }

    return identicalTo(to, tc, ctargs);
}

bool Type::isCompatibleToMultiProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeType::MultiProtocol) {
        return std::equal(protocols().begin(), protocols().end(), to.protocols().begin(), to.protocols().end(),
                          [ct, ctargs](const Type &a, const Type &b) {
                              return a.compatibleTo(b, ct, ctargs);
                          });
    }

    return std::all_of(to.protocols().begin(), to.protocols().end(), [this, ct](const Type &p) {
        return compatibleTo(p, ct);
    });
}

bool Type::isCompatibleToProtocol(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeType::Class) {
        for (Class *a = this->klass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.resolveOn(TypeContext(*this)).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                    return true;
                }
            }
        }
        return false;
    }
    if (type() == TypeType::ValueType || type() == TypeType::Enum) {
        for (auto &protocol : typeDefinition()->protocols()) {
            if (protocol.resolveOn(TypeContext(*this)).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                return true;
            }
        }
        return false;
    }
    if (type() == TypeType::Protocol) {
        return this->typeDefinition() == to.typeDefinition() && identicalGenericArguments(to, ct, ctargs);
    }
    return false;
}

bool Type::isCompatibleToCallable(const Type &to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeType::Callable) {
        if (genericArguments_[0].compatibleTo(to.genericArguments_[0], ct, ctargs)
            && to.genericArguments_.size() == genericArguments_.size()) {
            for (size_t i = 1; i < to.genericArguments_.size(); i++) {
                if (!to.genericArguments_[i].compatibleTo(genericArguments_[i], ct, ctargs)) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool Type::identicalTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const {
    if (ctargs != nullptr && to.type() == TypeType::LocalGenericVariable) {
        (*ctargs)[to.genericVariableIndex()].addType(*this, tc);
        return true;
    }

    if (type() == to.type()) {
        switch (type()) {
            case TypeType::Class:
            case TypeType::Protocol:
            case TypeType::ValueType:
                return typeDefinition() == to.typeDefinition()
                       && identicalGenericArguments(to, tc, ctargs);
            case TypeType::Callable:
                return to.genericArguments_.size() == this->genericArguments_.size()
                       && identicalGenericArguments(to, tc, ctargs);
            case TypeType::Optional:
            case TypeType::TypeAsValue:
                return genericArguments_[0].identicalTo(to.genericArguments_[0], tc, ctargs);
            case TypeType::Enum:
                return enumeration() == to.enumeration();
            case TypeType::GenericVariable:
            case TypeType::LocalGenericVariable:
                return resolveReferenceToBaseReferenceOnSuperArguments(tc).genericVariableIndex() ==
                       to.resolveReferenceToBaseReferenceOnSuperArguments(tc).genericVariableIndex();
            case TypeType::Something:
            case TypeType::Someobject:
            case TypeType::NoReturn:
                return true;
            case TypeType::Error:
                return errorType().identicalTo(to.errorType(), tc, ctargs) &&
                       errorEnum().identicalTo(to.errorEnum(), tc, ctargs);
            case TypeType::MultiProtocol:
                return std::equal(protocols().begin(), protocols().end(), to.protocols().begin(), to.protocols().end(),
                                  [&tc, ctargs](const Type &a, const Type &b) { return a.identicalTo(b, tc, ctargs); });
            case TypeType::StorageExpectation:
            case TypeType::Extension:
                return false;
        }
    }
    return false;
}

StorageType Type::storageType() const {
    if (forceBox_ || requiresBox()) {
        return StorageType::Box;
    }
    if (type() == TypeType::Optional) {
        return StorageType::SimpleOptional;
    }
    if (type() == TypeType::Error) {
        return StorageType::SimpleError;
    }
    return StorageType::Simple;
}

bool Type::requiresBox() const {
    switch (type()) {
        case TypeType::Error:
            return errorType().storageType() == StorageType::Box;
        case TypeType::Optional:
            return optionalType().storageType() == StorageType::Box;
        case TypeType::Something:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
            return true;
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
            return forceBox_;
        default:
            return false;
    }
}

bool Type::isReferencable() const {
    switch (type()) {
        case TypeType::Callable:
        case TypeType::Class:
        case TypeType::Someobject:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::TypeAsValue:
            return storageType() != StorageType::Simple;
        case TypeType::NoReturn:
        case TypeType::Optional:  // only reached in case of error, CompilerError will be/was raised
            return false;
        case TypeType::ValueType:
        case TypeType::Enum:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
        case TypeType::Something:
        case TypeType::Error:
            return true;
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            throw std::logic_error("isReferenceWorthy for StorageExpectation/Extension");
    }
}

std::string Type::typePackage() const {
    switch (this->type()) {
        case TypeType::Class:
        case TypeType::ValueType:
        case TypeType::Protocol:
        case TypeType::Enum:
            return typeDefinition()->package()->name();
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            throw std::logic_error("typePackage for StorageExpectation/Extension");
        default:
            return "";
    }
}

void Type::typeName(Type type, const TypeContext &typeContext, std::string &string, bool package) const {
    if (package) {
        string.append(type.typePackage());
    }

    switch (type.type()) {
        case TypeType::Optional:
            string.append("🍬");
            typeName(type.genericArguments_[0], typeContext, string, package);
            return;
        case TypeType::TypeAsValue:
            switch (type.typeOfTypeValue().type()) {
                case TypeType::Class:
                    string.append("🐇");
                    break;
                case TypeType::ValueType:
                    string.append("🕊");
                    break;
                case TypeType::Enum:
                    string.append("🦃");
                    break;
                case TypeType::Protocol:
                    string.append("🐊");
                    break;
                default:
                    break;
            }
            typeName(type.typeOfTypeValue(), typeContext, string, package);
            return;
        case TypeType::Class:
        case TypeType::Protocol:
        case TypeType::Enum:
        case TypeType::ValueType:
            string.append(utf8(type.typeDefinition()->name()));
            break;
        case TypeType::MultiProtocol:
            string.append("🍱");
            for (auto &protocol : type.protocols()) {
                typeName(protocol, typeContext, string, package);
            }
            string.append("🍱");
            return;
        case TypeType::NoReturn:
            string.append("(no return)");
            return;
        case TypeType::Something:
            string.append("⚪️");
            return;
        case TypeType::Someobject:
            string.append("🔵");
            return;
        case TypeType::Callable:
            string.append("🍇");

            for (size_t i = 1; i < type.genericArguments_.size(); i++) {
                typeName(type.genericArguments_[i], typeContext, string, package);
            }

            if (type.genericArguments().front().type() != TypeType::NoReturn) {
                string.append("➡️");
                typeName(type.genericArguments().front(), typeContext, string, package);
            }
            string.append("🍉");
            return;
        case TypeType::Error:
            string.append("🚨");
            typeName(type.errorEnum(), typeContext, string, package);
            typeName(type.errorType(), typeContext, string, package);
            return;
        case TypeType::GenericVariable:
            if (typeContext.calleeType().canHaveGenericArguments()) {
                auto str = typeContext.calleeType().typeDefinition()->findGenericName(type.genericVariableIndex());
                string.append(utf8(str));
            }
            else {
                string.append("T" + std::to_string(type.genericVariableIndex()) + "?");
            }
            return;
        case TypeType::LocalGenericVariable:
            if (typeContext.function() == type.localResolutionConstraint_) {
                string.append(utf8(typeContext.function()->findGenericName(type.genericVariableIndex())));
            }
            else {
                string.append("L" + std::to_string(type.genericVariableIndex()) + "?");
            }
            return;
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            return;
    }

    if (type.canHaveGenericArguments()) {
        for (size_t i = type.typeDefinition()->superGenericArguments().size(); i < type.genericArguments().size(); i++) {
            string.append("🐚");
            typeName(type.genericArguments()[i], typeContext, string, package);
        }
    }
}

std::string Type::toString(const TypeContext &typeContext, bool package) const {
    std::string string;
    typeName(*this, typeContext, string, package);
    return string;
}

}  // namespace EmojicodeCompiler
