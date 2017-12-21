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
#include "EmojicodeCompiler.hpp"
#include "Enum.hpp"
#include "Extension.hpp"
#include "Functions/Function.hpp"
#include "Package/Package.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"
#include "ValueType.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

Type::Type(Protocol *protocol, bool optional)
    : typeContent_(TypeType::Protocol), typeDefinition_(protocol), optional_(optional) {}

Type::Type(Enum *enumeration, bool optional)
    : typeContent_(TypeType::Enum), typeDefinition_(enumeration), optional_(optional) {}

Type::Type(Extension *extension)
    : typeContent_(TypeType::Extension), typeDefinition_(extension), optional_(false) {}

Type::Type(ValueType *valueType, bool optional)
: typeContent_(TypeType::ValueType), typeDefinition_(valueType), optional_(optional), mutable_(false) {
    for (size_t i = 0; i < valueType->genericParameterCount(); i++) {
        genericArguments_.emplace_back(false, i, valueType);
    }
}

Type::Type(Class *klass, bool optional) : typeContent_(TypeType::Class), typeDefinition_(klass), optional_(optional) {
    for (size_t i = 0; i < klass->genericParameterCount() + klass->superGenericArguments().size(); i++) {
        genericArguments_.emplace_back(false, i, klass);
    }
}

Class* Type::eclass() const {
    return dynamic_cast<Class *>(typeDefinition_);
}

Protocol* Type::protocol() const {
    return dynamic_cast<Protocol *>(typeDefinition_);
}

Enum* Type::eenum() const {
    return dynamic_cast<Enum *>(typeDefinition_);
}

ValueType* Type::valueType() const {
    return dynamic_cast<ValueType *>(typeDefinition_);
}

TypeDefinition* Type::typeDefinition() const  {
    return typeDefinition_;
}

bool Type::canHaveGenericArguments() const {
    return type() == TypeType::Class || type() == TypeType::Protocol || type() == TypeType::ValueType;
}

void Type::sortMultiProtocolType() {
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

bool Type::allowsMetaType() const {
    return type() == TypeType::Class || type() == TypeType::Enum || type() == TypeType::ValueType;
}

Type Type::resolveReferenceToBaseReferenceOnSuperArguments(const TypeContext &typeContext) const {
    TypeDefinition *c = typeContext.calleeType().typeDefinition();
    Type t = *this;

    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeType::GenericVariable && c->canBeUsedToResolve(t.typeDefinition()) &&
           t.genericVariableIndex() < c->superGenericArguments().size()) {
        Type tn = c->superGenericArguments()[t.genericVariableIndex()];
        if (tn.type() == TypeType::GenericVariable && tn.genericVariableIndex() == t.genericVariableIndex()
            && tn.typeDefinition() == t.typeDefinition()) {
            break;
        }
        t = tn;
    }
    return t;
}

Type Type::resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext) const {
    if (typeContext.calleeType().type() == TypeType::NoReturn) {
        return *this;
    }
    TypeDefinition *c = typeContext.calleeType().typeDefinition();
    Type t = *this;
    if (type() == TypeType::NoReturn) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeType::GenericVariable && t.genericVariableIndex() < c->superGenericArguments().size()) {
        t = c->superGenericArguments()[t.genericArgumentIndex_];
    }
    while (t.type() == TypeType::LocalGenericVariable && typeContext.function() == t.localResolutionConstraint_) {
        t = typeContext.function()->constraintForIndex(t.genericArgumentIndex_);
    }
    while (t.type() == TypeType::GenericVariable
           && typeContext.calleeType().typeDefinition()->canBeUsedToResolve(t.typeDefinition())) {
        t = typeContext.calleeType().typeDefinition()->constraintForIndex(t.genericArgumentIndex_);
    }

    if (optional) {
        t.setOptional();
    }
    if (box) {
        t.forceBox_ = true;
    }
    return t;
}

Type Type::resolveOn(const TypeContext &typeContext) const {
    Type t = *this;
    if (type() == TypeType::NoReturn) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    while (t.type() == TypeType::LocalGenericVariable && typeContext.function() == t.localResolutionConstraint_
           && typeContext.functionGenericArguments() != nullptr) {
        t = (*typeContext.functionGenericArguments())[t.genericVariableIndex()];
    }

    if (typeContext.calleeType().canHaveGenericArguments()) {
        while (t.type() == TypeType::GenericVariable &&
               typeContext.calleeType().typeDefinition()->canBeUsedToResolve(t.typeDefinition())) {
            Type tn = typeContext.calleeType().genericArguments_[t.genericVariableIndex()];
            if (tn.type() == TypeType::GenericVariable && tn.genericVariableIndex() == t.genericVariableIndex()
                && tn.typeDefinition() == t.typeDefinition()) {
                break;
            }
            t = tn;
        }
    }

    if (optional) {
        t.setOptional();
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
        return to.type() == TypeType::Error && genericArguments()[0].identicalTo(to.genericArguments()[0], tc, ctargs)
                && genericArguments()[1].compatibleTo(to.genericArguments()[1], tc);
    }
    if (to.type() == TypeType::Something) {
        return true;
    }
    if (to.meta_ != meta_) {
        return false;
    }
    if (this->optional() && !to.optional()) {
        return false;
    }

    if ((this->type() == TypeType::GenericVariable && to.type() == TypeType::GenericVariable) ||
        (this->type() == TypeType::LocalGenericVariable && to.type() == TypeType::LocalGenericVariable)) {
        return (this->genericVariableIndex() == to.genericVariableIndex() &&
                this->typeDefinition() == to.typeDefinition()) ||
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
        case TypeType::GenericVariable:
            return compatibleTo(to.resolveOnSuperArgumentsAndConstraints(tc), tc, ctargs);
        case TypeType::LocalGenericVariable:
            if (ctargs != nullptr) {
                (*ctargs)[to.genericVariableIndex()].addType(*this, tc);
                return true;
            }
            return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(tc), tc, ctargs);
        case TypeType::Class:
            return type() == TypeType::Class && eclass()->inheritsFrom(to.eclass()) &&
                identicalGenericArguments(to, tc, ctargs);
        case TypeType::ValueType:
            return type() == TypeType::ValueType && typeDefinition() == to.typeDefinition() &&
                identicalGenericArguments(to, tc, ctargs);
        case TypeType::Enum:
            return type() == TypeType::Enum && eenum() == to.eenum();
        case TypeType::Someobject:
            return type() == TypeType::Class || type() == TypeType::Someobject;
        case TypeType::Error:
            return compatibleTo(to.genericArguments()[1], tc);
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
        for (Class *a = this->eclass(); a != nullptr; a = a->superclass()) {
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
    if (optional() != to.optional()) {
        return false;
    }
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
            case TypeType::Enum:
                return eenum() == to.eenum();
            case TypeType::GenericVariable:
            case TypeType::LocalGenericVariable:
                return resolveReferenceToBaseReferenceOnSuperArguments(tc).genericVariableIndex() ==
                to.resolveReferenceToBaseReferenceOnSuperArguments(tc).genericVariableIndex();
            case TypeType::Something:
            case TypeType::Someobject:
            case TypeType::NoReturn:
                return true;
            case TypeType::Error:
                return genericArguments_[0].identicalTo(to.genericArguments_[0], tc, ctargs) &&
                genericArguments_[1].identicalTo(to.genericArguments_[1], tc, ctargs);
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


// MARK: Storage

StorageType Type::storageType() const {
    if (forceBox_ || requiresBox()) {
        return StorageType::Box;
    }
    if (type() == TypeType::Error) {
        return StorageType::SimpleOptional;
    }
    return optional() ? StorageType::SimpleOptional : StorageType::Simple;
}

bool Type::requiresBox() const {
    switch (type()) {
        case TypeType::ValueType:
        case TypeType::Enum:
        case TypeType::Callable:
        case TypeType::Class:
        case TypeType::Someobject:
        case TypeType::NoReturn:
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            return false;
        case TypeType::Error:
            return genericArguments()[1].storageType() == StorageType::Box;
        case TypeType::Something:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
            return true;
    }
}

bool Type::isReferencable() const {
    switch (type()) {
        case TypeType::Callable:
        case TypeType::Class:
        case TypeType::Someobject:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
            return storageType() != StorageType::Simple;
        case TypeType::NoReturn:
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

template <typename T, typename... Us>
void Type::objectVariableRecords(int index, std::vector<T> *information, Us... args) const {
    switch (type()) {
        case TypeType::ValueType: {
            if (storageType() == StorageType::Box) {
                information->push_back(T(index, ObjectVariableType::Box, args...));
                return;
            }

            auto optional = storageType() == StorageType::SimpleOptional;
            auto size = information->size();
            for (auto &variable : valueType()->instanceScope().map()) {
                // TODO:               auto vindex = index + variable.second.id() + (optional ? 1 : 0);
                // variable.second.type().objectVariableRecords(vindex, information, args...);
            }
            if (optional) {
                auto info = T(static_cast<unsigned int>(information->size() - size), index,
                              ObjectVariableType::ConditionalSkip, args...);
                information->insert(information->begin() + size, info);
            }
            return;
        }
        case TypeType::Class:
        case TypeType::Someobject:
        case TypeType::Callable:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
        case TypeType::Something:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::Error:  // Is or may be an object pointer
            switch (storageType()) {
                case StorageType::SimpleOptional:
                    information->push_back(T(index + 1, index, ObjectVariableType::Condition, args...));
                    return;
                case StorageType::Simple:
                    information->push_back(T(index, ObjectVariableType::Simple, args...));
                    return;
                case StorageType::Box:
                    information->push_back(T(index, ObjectVariableType::Box, args...));
                    return;
                default:
                    throw std::domain_error("invalid storage type");
            }
        case TypeType::Enum:
        case TypeType::NoReturn:
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            return;  // Can't be object pointer
    }
}

template void Type::objectVariableRecords(int, std::vector<ObjectVariableInformation>*) const;

// MARK: Type Visulisation

std::string Type::typePackage() const {
    switch (this->type()) {
        case TypeType::Class:
        case TypeType::ValueType:
        case TypeType::Protocol:
        case TypeType::Enum:
            return typeDefinition()->package()->name();
        case TypeType::NoReturn:
        case TypeType::Something:
        case TypeType::Someobject:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::Callable:
        case TypeType::MultiProtocol:  // should actually never come in here
        case TypeType::Error:
            return "";
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            throw std::logic_error("typePackage for StorageExpectation/Extension");
    }
}

void Type::typeName(Type type, const TypeContext &typeContext, std::string &string, bool package) const {
    if (type.meta_) {
        string.append("🔳");
    }


    if (type.optional()) {
        string.append("🍬");
    }

    if (package) {
        string.append(type.typePackage());
    }

    switch (type.type()) {
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
            string.append("✨");
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

            string.append("➡️");
            typeName(type.genericArguments_[0], typeContext, string, package);
            string.append("🍉");
            return;
        case TypeType::Error:
            string.append("🚨");
            typeName(type.genericArguments_[0], typeContext, string, package);
            typeName(type.genericArguments_[1], typeContext, string, package);
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
            if (typeContext.function() != nullptr) {
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
