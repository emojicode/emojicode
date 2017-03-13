//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Type.hpp"
#include "Class.hpp"
#include "Function.hpp"
#include "../utf8.h"
#include "EmojicodeCompiler.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"
#include "ValueType.hpp"
#include <cstring>
#include <vector>
#include <algorithm>

Class *CL_STRING;
Class *CL_LIST;
Class *CL_DATA;
Class *CL_DICTIONARY;
Protocol *PR_ENUMERATEABLE;
Protocol *PR_ENUMERATOR;
ValueType *VT_BOOLEAN;
ValueType *VT_SYMBOL;
ValueType *VT_INTEGER;
ValueType *VT_DOUBLE;

Type::Type(Protocol *p, bool o) : typeDefinition_(p), typeContent_(TypeContent::Protocol), optional_(o)  {
}

Type::Type(Enum *e, bool o) : typeDefinition_(e), typeContent_(TypeContent::Enum), optional_(o) {
}

Type::Type(ValueType *v, bool o, bool reference, bool isMutable)
: typeDefinition_(v), typeContent_(TypeContent::ValueType), optional_(o), mutable_(isMutable) {
}

Type::Type(Class *c, bool o) : typeDefinition_(c), typeContent_(TypeContent::Class), optional_(o) {
    for (int i = 0; i < c->numberOfGenericArgumentsWithSuperArguments(); i++) {
        genericArguments_.push_back(Type(TypeContent::Reference, false, i, c));
    }
}

Class* Type::eclass() const {
    return static_cast<Class *>(typeDefinition_);
}

Protocol* Type::protocol() const {
    return static_cast<Protocol *>(typeDefinition_);
}

Enum* Type::eenum() const {
    return static_cast<Enum *>(typeDefinition_);
}

ValueType* Type::valueType() const {
    return static_cast<ValueType *>(typeDefinition_);
}

TypeDefinition* Type::typeDefinition() const  {
    return typeDefinition_;
}

bool Type::canHaveGenericArguments() const {
    return type() == TypeContent::Class || type() == TypeContent::Protocol;
}

TypeDefinitionFunctional* Type::typeDefinitionFunctional() const {
    return static_cast<TypeDefinitionFunctional *>(typeDefinition_);
}

Type Type::copyWithoutOptional() const {
    Type type = *this;
    type.optional_ = false;
    return type;
}

void Type::sortMultiProtocolType() {
    std::sort(genericArguments_.begin(), genericArguments_.end(), [](const Type &a, const Type &b) {
        return a.protocol()->index < b.protocol()->index;
    });
}

int Type::reference() const {
    if (type() != TypeContent::Reference && type() != TypeContent::LocalReference) {
        throw std::domain_error("Tried to get reference from non-reference type");
    }
    return reference_;
}

bool Type::allowsMetaType() {
    return type() == TypeContent::Class || type() == TypeContent::Enum || type() == TypeContent::ValueType;
}

Type Type::resolveReferenceToBaseReferenceOnSuperArguments(const TypeContext &typeContext) const {
    TypeDefinitionFunctional *c = typeContext.calleeType().typeDefinitionFunctional();
    Type t = *this;

    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->ownGenericArgumentVariables().size();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeContent::Reference && c->canBeUsedToResolve(t.resolutionConstraint_) &&
           t.reference() < maxReferenceForSuper) {
        Type tn = c->superGenericArguments()[t.reference()];
        if (tn.type() == TypeContent::Reference && tn.reference() == t.reference()
            && tn.resolutionConstraint_ == t.resolutionConstraint_) {
            break;
        }
        t = tn;
    }
    return t;
}

Type Type::resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext, bool resolveSelf) const {
    if (typeContext.calleeType().type() == TypeContent::Nothingness) {
        return *this;
    }
    TypeDefinitionFunctional *c = typeContext.calleeType().typeDefinitionFunctional();
    Type t = *this;
    if (type() == TypeContent::Nothingness) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    if (resolveSelf && t.type() == TypeContent::Self) {
        t = typeContext.calleeType();
    }

    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - - c->ownGenericArgumentVariables().size();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeContent::Reference && t.reference() < maxReferenceForSuper) {
        t = c->superGenericArguments()[t.reference()];
    }
    while (t.type() == TypeContent::LocalReference && typeContext.function() == t.localResolutionConstraint_) {
        t = typeContext.function()->genericArgumentConstraints[t.reference()];
    }
    while (t.type() == TypeContent::Reference
           && typeContext.calleeType().typeDefinitionFunctional()->canBeUsedToResolve(t.resolutionConstraint_)) {
        t = typeContext.calleeType().typeDefinitionFunctional()->genericArgumentConstraints()[t.reference()];
    }

    if (optional) {
        t.setOptional();
    }
    if (box) {
        t.forceBox_ = true;
    }
    return t;
}

Type Type::resolveOn(const TypeContext &typeContext, bool resolveSelf) const {
    Type t = *this;
    if (type() == TypeContent::Nothingness) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    if (resolveSelf && t.type() == TypeContent::Self && typeContext.calleeType().resolveSelfOn_) {
        t = typeContext.calleeType();
    }

    while (t.type() == TypeContent::LocalReference && typeContext.function() == t.localResolutionConstraint_
           && typeContext.functionGenericArguments()) {
        t = (*typeContext.functionGenericArguments())[t.reference()];
    }

    if (typeContext.calleeType().canHaveGenericArguments()) {
        while (t.type() == TypeContent::Reference &&
               typeContext.calleeType().typeDefinitionFunctional()->canBeUsedToResolve(t.resolutionConstraint_)) {
            Type tn = typeContext.calleeType().genericArguments_[t.reference()];
            if (tn.type() == TypeContent::Reference && tn.reference() == t.reference()
                && tn.resolutionConstraint_ == t.resolutionConstraint_) {
                break;
            }
            t = tn;
        }
    }

    if (optional) {
        t.setOptional();
    }

    if (t.canHaveGenericArguments()) {
        for (size_t i = 0; i < t.typeDefinitionFunctional()->numberOfGenericArgumentsWithSuperArguments(); i++) {
            t.genericArguments_[i] = t.genericArguments_[i].resolveOn(typeContext);
        }
    }
    else if (t.type() == TypeContent::Callable) {
        for (Type &genericArgument : t.genericArguments_) {
            genericArgument = genericArgument.resolveOn(typeContext);
        }
    }

    if (box) {
        t.forceBox_ = true;
    }
    return t;
}

bool Type::identicalGenericArguments(Type to, const TypeContext &typeContext, std::vector<CommonTypeFinder> *ctargs) const {
    if (!to.typeDefinitionFunctional()->ownGenericArgumentVariables().empty()) {
        for (size_t l = to.typeDefinitionFunctional()->ownGenericArgumentVariables().size(),
             i = to.typeDefinitionFunctional()->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
            if (!this->genericArguments_[i].identicalTo(to.genericArguments_[i], typeContext, ctargs)) {
                return false;
            }
        }
    }
    return true;
}

bool Type::compatibleTo(Type to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeContent::Error) {
        return to.type() == TypeContent::Error && genericArguments()[0].identicalTo(to.genericArguments()[0], ct, ctargs)
                && genericArguments()[1].compatibleTo(to.genericArguments()[1], ct);
    }
    if (to.type() == TypeContent::Something) {
        return true;
    }
    if (to.meta_ != meta_) {
        return false;
    }
    if (this->optional() && !to.optional()) {
        return false;
    }

    if (to.type() == TypeContent::Someobject && this->type() == TypeContent::Class) {
        return true;
    }
    if (to.type() == TypeContent::Error) {
        return compatibleTo(to.genericArguments()[1], ct);
    }
    if (this->type() == TypeContent::Class && to.type() == TypeContent::Class) {
        return this->eclass()->inheritsFrom(to.eclass()) && identicalGenericArguments(to, ct, ctargs);
    }
    if ((this->type() == TypeContent::Protocol && to.type() == TypeContent::Protocol) ||
        (this->type() == TypeContent::ValueType && to.type() == TypeContent::ValueType)) {
        return this->typeDefinition() == to.typeDefinition() && identicalGenericArguments(to, ct, ctargs);
    }
    if (type() == TypeContent::MultiProtocol && to.type() == TypeContent::MultiProtocol) {
        return std::equal(protocols().begin(), protocols().end(), to.protocols().begin(), to.protocols().end(),
                          [ct, ctargs](const Type &a, const Type &b) {
                              return a.compatibleTo(b, ct, ctargs);
                          });
    }
    if (to.type() == TypeContent::MultiProtocol) {
        return std::all_of(to.protocols().begin(), to.protocols().end(), [this, ct](const Type &p) {
            return compatibleTo(p, ct);
        });
    }
    if (type() == TypeContent::Class && to.type() == TypeContent::Protocol) {
        for (Class *a = this->eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.resolveOn(*this).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                    return true;
                }
            }
        }
        return false;
    }
    if ((type() == TypeContent::ValueType || type() == TypeContent::Enum) && to.type() == TypeContent::Protocol) {
        for (auto &protocol : typeDefinitionFunctional()->protocols()) {
            if (protocol.resolveOn(*this).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                return true;
            }
        }
        return false;
    }
    if (this->type() == TypeContent::Nothingness) {
        return to.optional() || to.type() == TypeContent::Nothingness;
    }
    if (this->type() == TypeContent::Enum && to.type() == TypeContent::Enum) {
        return this->eenum() == to.eenum();
    }
    if ((this->type() == TypeContent::Reference && to.type() == TypeContent::Reference) ||
        (this->type() == TypeContent::LocalReference && to.type() == TypeContent::LocalReference)) {
        return (this->reference() == to.reference() && this->resolutionConstraint_ == to.resolutionConstraint_) ||
        this->resolveOnSuperArgumentsAndConstraints(ct)
        .compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (this->type() == TypeContent::Reference) {
        return this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (to.type() == TypeContent::Reference) {
        return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (this->type() == TypeContent::LocalReference) {
        return ctargs != nullptr || this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (to.type() == TypeContent::LocalReference) {
        if (ctargs != nullptr) {
            (*ctargs)[to.reference()].addType(*this, ct);
            return true;
        }
        return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (to.type() == TypeContent::Self) {
        return this->type() == to.type();
    }
    if (this->type() == TypeContent::Self) {
        return this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (this->type() == TypeContent::Callable && to.type() == TypeContent::Callable) {
        if (this->genericArguments_[0].compatibleTo(to.genericArguments_[0], ct, ctargs)
            && to.genericArguments_.size() == this->genericArguments_.size()) {
            for (size_t i = 1; i < to.genericArguments_.size(); i++) {
                if (!to.genericArguments_[i].compatibleTo(this->genericArguments_[i], ct, ctargs)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    return this->type() == to.type();
}

bool Type::identicalTo(Type to, const TypeContext &tc, std::vector<CommonTypeFinder> *ctargs) const {
    if (optional() != to.optional()) {
        return false;
    }
    if (ctargs != nullptr && to.type() == TypeContent::LocalReference) {
        (*ctargs)[to.reference()].addType(*this, tc);
        return true;
    }

    if (type() == to.type()) {
        switch (type()) {
            case TypeContent::Class:
            case TypeContent::Protocol:
            case TypeContent::ValueType:
                return typeDefinitionFunctional() == to.typeDefinitionFunctional()
                && identicalGenericArguments(to, tc, ctargs);
            case TypeContent::Callable:
                return to.genericArguments_.size() == this->genericArguments_.size()
                && identicalGenericArguments(to, tc, ctargs);
            case TypeContent::Enum:
                return eenum() == to.eenum();
            case TypeContent::Reference:
            case TypeContent::LocalReference:
                return resolveReferenceToBaseReferenceOnSuperArguments(tc).reference() ==
                to.resolveReferenceToBaseReferenceOnSuperArguments(tc).reference();
            case TypeContent::Self:
            case TypeContent::Something:
            case TypeContent::Someobject:
            case TypeContent::Nothingness:
                return true;
            case TypeContent::Error:
                return genericArguments_[0].identicalTo(to.genericArguments_[0], tc, ctargs) &&
                genericArguments_[1].identicalTo(to.genericArguments_[1], tc, ctargs);
            case TypeContent::MultiProtocol:
                return std::equal(protocols().begin(), protocols().end(), to.protocols().begin(), to.protocols().end(),
                                  [&tc, ctargs](const Type &a, const Type &b) { return a.identicalTo(b, tc, ctargs); });
        }
    }
    return false;
}


// MARK: Storage

int Type::size() const {
    int basesize = 0;
    switch (storageType()) {
        case StorageType::SimpleOptional:
            basesize = 1;
        case StorageType::Simple:
            switch (type()) {
                case TypeContent::ValueType:
                case TypeContent::Enum:
                    return basesize + typeDefinition()->size();
                case TypeContent::Callable:
                case TypeContent::Class:
                case TypeContent::Someobject:
                case TypeContent::Self:
                    return basesize + 1;
                case TypeContent::Error:
                    if (genericArguments()[1].storageType() == StorageType::SimpleOptional) {
                        return std::max(2, genericArguments()[1].size());
                    }
                    return std::max(1, genericArguments()[1].size()) + basesize;
                case TypeContent::Nothingness:
                    return 0;
                default:
                    throw std::logic_error("Type is wrongly simply stored");
            }
        case StorageType::Box:
            return 4;
        default:
            throw std::logic_error("Type has invalid storage type");
    }
}

StorageType Type::storageType() const {
    if (forceBox_ || requiresBox()) {
        return StorageType::Box;
    }
    if (type() == TypeContent::Error) {
        return StorageType::SimpleOptional;
    }
    return optional() ? StorageType::SimpleOptional : StorageType::Simple;
}

EmojicodeInstruction Type::boxIdentifier() const {
    EmojicodeInstruction value;
    switch (type()) {
        case TypeContent::ValueType:
        case TypeContent::Enum:
            value = valueType()->boxIdentifier();
            break;
        case TypeContent::Callable:
        case TypeContent::Class:
        case TypeContent::Someobject:
            value = T_OBJECT;
            break;
        case TypeContent::Nothingness:
        case TypeContent::Protocol:
        case TypeContent::MultiProtocol:
        case TypeContent::Something:
        case TypeContent::Reference:
        case TypeContent::LocalReference:
        case TypeContent::Self:
        case TypeContent::Error:
            return 0;  // This can only be executed in the case of a semantic error, return any value
        case TypeContent::StorageExpectation:
            throw std::logic_error("Box identifier for StorageExpectation");
    }
    return meta() ? (value | META_MASK) : value;
}

bool Type::requiresBox() const {
    switch (type()) {
        case TypeContent::ValueType:
        case TypeContent::Enum:
        case TypeContent::Callable:
        case TypeContent::Class:
        case TypeContent::Someobject:
        case TypeContent::Self:
        case TypeContent::Nothingness:
        case TypeContent::StorageExpectation:
            return false;
        case TypeContent::Error:
            return genericArguments()[1].storageType() == StorageType::Box;
        case TypeContent::Something:
        case TypeContent::Reference:
        case TypeContent::LocalReference:
        case TypeContent::Protocol:
        case TypeContent::MultiProtocol:
            return true;
    }
}

bool Type::isReferenceWorthy() const {
    switch (type()) {
        case TypeContent::Callable:
        case TypeContent::Class:
        case TypeContent::Someobject:
        case TypeContent::Reference:
        case TypeContent::LocalReference:
        case TypeContent::Self:
            return storageType() != StorageType::Simple;
        case TypeContent::Nothingness:
            return false;
        case TypeContent::ValueType:
        case TypeContent::Enum:
        case TypeContent::Protocol:
        case TypeContent::MultiProtocol:
        case TypeContent::Something:
        case TypeContent::Error:
            return true;
        case TypeContent::StorageExpectation:
            throw std::logic_error("isReferenceWorthy for StorageExpectation");
    }
}

template <typename T, typename... Us>
void Type::objectVariableRecords(int index, std::vector<T> &information, Us... args) const {
    switch (type()) {
        case TypeContent::ValueType: {
            if (storageType() == StorageType::Box) {
                information.push_back(T(index, ObjectVariableType::Box, args...));
                return;
            }

            auto optional = storageType() == StorageType::SimpleOptional;
            auto size = information.size();
            for (auto variable : valueType()->instanceScope().map()) {
                auto vindex = index + variable.second.id() + (optional ? 1 : 0);
                variable.second.type().objectVariableRecords(vindex, information, args...);
            }
            if (optional) {
                auto info = T(static_cast<unsigned int>(information.size() - size), index,
                              ObjectVariableType::ConditionalSkip, args...);
                information.insert(information.begin() + size, info);
            }
            return;
        }
        case TypeContent::Class:
        case TypeContent::Self:
        case TypeContent::Someobject:
        case TypeContent::Callable:
        case TypeContent::Protocol:
        case TypeContent::MultiProtocol:
        case TypeContent::Something:
        case TypeContent::Reference:
        case TypeContent::LocalReference:
        case TypeContent::Error:
            switch (storageType()) {
                case StorageType::SimpleOptional:
                    information.push_back(T(index + 1, index, ObjectVariableType::Condition, args...));
                    return;
                case StorageType::Simple:
                    information.push_back(T(index, ObjectVariableType::Simple, args...));
                    return;
                case StorageType::Box:
                    information.push_back(T(index, ObjectVariableType::Box, args...));
                    return;
                default:
                    throw std::domain_error("invalid storage type");
            }
        case TypeContent::Enum:
        case TypeContent::Nothingness:
            return;  // Can't be object pointer
    }
}

template void Type::objectVariableRecords(int, std::vector<ObjectVariableInformation>&) const;
template void Type::objectVariableRecords(int, std::vector<FunctionObjectVariableInformation>&,
                                          InstructionCount, InstructionCount) const;

// MARK: Type Visulisation

std::string Type::typePackage() {
    switch (this->type()) {
        case TypeContent::Class:
        case TypeContent::ValueType:
        case TypeContent::Protocol:
        case TypeContent::Enum:
            return this->typeDefinition()->package()->name();
        case TypeContent::Nothingness:
        case TypeContent::Something:
        case TypeContent::Someobject:
        case TypeContent::Reference:
        case TypeContent::LocalReference:
        case TypeContent::Callable:
        case TypeContent::Self:
        case TypeContent::MultiProtocol:  // should actually never come in here
        case TypeContent::Error:
            return "";
        case TypeContent::StorageExpectation:
            throw std::logic_error("typePackage for StorageExpectation");
    }
}

void Type::typeName(Type type, const TypeContext &typeContext, bool includePackageAndOptional, std::string &string) const {
    if (type.meta_) {
        string.append("🔳");
    }

    if (includePackageAndOptional) {
        if (type.optional()) {
            string.append("🍬");
        }

        string.append(type.typePackage());
    }

    switch (type.type()) {
        case TypeContent::Class:
        case TypeContent::Protocol:
        case TypeContent::Enum:
        case TypeContent::ValueType:
            string.append(type.typeDefinition()->name().utf8());
            break;
        case TypeContent::MultiProtocol:
            string.append("🍱");
            for (auto &protocol : type.protocols()) {
                typeName(protocol, typeContext, includePackageAndOptional, string);
            }
            string.append("🍱");
            return;
        case TypeContent::Nothingness:
            string.append("✨");
            return;
        case TypeContent::Something:
            string.append("⚪️");
            return;
        case TypeContent::Someobject:
            string.append("🔵");
            return;
        case TypeContent::Self:
            string.append("🐕");
            return;
        case TypeContent::Callable:
            string.append("🍇");

            for (size_t i = 1; i < type.genericArguments_.size(); i++) {
                typeName(type.genericArguments_[i], typeContext, includePackageAndOptional, string);
            }

            string.append("➡️");
            typeName(type.genericArguments_[0], typeContext, includePackageAndOptional, string);
            string.append("🍉");
            return;
        case TypeContent::Error:
            string.append("🚨");
            typeName(type.genericArguments_[0], typeContext, includePackageAndOptional, string);
            typeName(type.genericArguments_[1], typeContext, includePackageAndOptional, string);
            return;
        case TypeContent::Reference: {
            if (typeContext.calleeType().type() == TypeContent::Class) {
                Class *eclass = typeContext.calleeType().eclass();
                do {
                    for (auto it : eclass->ownGenericArgumentVariables()) {
                        if (it.second.reference() == type.reference()) {
                            string.append(it.first.utf8());
                            return;
                        }
                    }
                } while ((eclass = eclass->superclass()));
            }
            else if (typeContext.calleeType().canHaveGenericArguments()) {
                for (auto it : typeContext.calleeType().typeDefinitionFunctional()->ownGenericArgumentVariables()) {
                    if (it.second.reference() == type.reference()) {
                        string.append(it.first.utf8());
                        return;
                    }
                }
            }

            string.append("T" + std::to_string(type.reference()) + "?");
            return;
        }
        case TypeContent::LocalReference:
            if (typeContext.function()) {
                for (auto it : typeContext.function()->genericArgumentVariables) {
                    if (it.second.reference() == type.reference()) {
                        string.append(it.first.utf8());
                        return;
                    }
                }
            }

            string.append("L" + std::to_string(type.reference()) + "?");
            return;
    }

    if (type.canHaveGenericArguments()) {
        auto typeDef = type.typeDefinitionFunctional();
        if (typeDef->ownGenericArgumentVariables().empty()) {
            return;
        }

        for (auto &argumentType : type.genericArguments()) {
            string.append("🐚");
            typeName(argumentType, typeContext, includePackageAndOptional, string);
        }
    }
}

std::string Type::toString(const TypeContext &typeContext, bool optionalAndPackages) const {
    std::string string;
    typeName(*this, typeContext, optionalAndPackages, string);
    return string;
}
