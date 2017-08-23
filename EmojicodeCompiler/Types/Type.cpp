//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Type.hpp"
#include "../../utf8.h"
#include "../EmojicodeCompiler.hpp"
#include "../Function.hpp"
#include "../Types/Class.hpp"
#include "../Types/Enum.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/TypeContext.hpp"
#include "../Types/ValueType.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

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

Type::Type(Protocol *protocol, bool o) : typeContent_(TypeType::Protocol), typeDefinition_(protocol), optional_(o) {
}

Type::Type(Enum *enumeration, bool o) : typeContent_(TypeType::Enum), typeDefinition_(enumeration), optional_(o) {
}

Type::Type(Extension *extension) : typeContent_(TypeType::Extension), typeDefinition_(extension), optional_(false) {
}

Type::Type(ValueType *valueType, bool o)
: typeContent_(TypeType::ValueType), typeDefinition_(valueType), optional_(o), mutable_(false) {
    for (size_t i = 0; i < valueType->numberOfGenericArgumentsWithSuperArguments(); i++) {
        genericArguments_.emplace_back(TypeType::GenericVariable, false, i, valueType);
    }
}

Type::Type(Class *c, bool o) : typeContent_(TypeType::Class), typeDefinition_(c), optional_(o) {
    for (size_t i = 0; i < c->numberOfGenericArgumentsWithSuperArguments(); i++) {
        genericArguments_.emplace_back(TypeType::GenericVariable, false, i, c);
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
        return a.protocol()->index < b.protocol()->index;
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

    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->ownGenericArgumentVariables().size();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeType::GenericVariable && c->canBeUsedToResolve(t.typeDefinition()) &&
           t.genericVariableIndex() < maxReferenceForSuper) {
        Type tn = c->superGenericArguments()[t.genericVariableIndex()];
        if (tn.type() == TypeType::GenericVariable && tn.genericVariableIndex() == t.genericVariableIndex()
            && tn.typeDefinition() == t.typeDefinition()) {
            break;
        }
        t = tn;
    }
    return t;
}

Type Type::resolveOnSuperArgumentsAndConstraints(const TypeContext &typeContext, bool resolveSelf) const {
    if (typeContext.calleeType().type() == TypeType::Nothingness) {
        return *this;
    }
    TypeDefinition *c = typeContext.calleeType().typeDefinition();
    Type t = *this;
    if (type() == TypeType::Nothingness) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    if (resolveSelf && t.type() == TypeType::Self) {
        t = typeContext.calleeType();
    }

    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->ownGenericArgumentVariables().size();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeType::GenericVariable && t.genericVariableIndex() < maxReferenceForSuper) {
        t = c->superGenericArguments()[t.genericVariableIndex()];
    }
    while (t.type() == TypeType::LocalGenericVariable && typeContext.function() == t.localResolutionConstraint_) {
        t = typeContext.function()->genericArgumentConstraints[t.genericVariableIndex()];
    }
    while (t.type() == TypeType::GenericVariable
           && typeContext.calleeType().typeDefinition()->canBeUsedToResolve(t.typeDefinition())) {
        t = typeContext.calleeType().typeDefinition()->genericArgumentConstraints()[t.genericVariableIndex()];
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
    if (type() == TypeType::Nothingness) {
        return t;
    }
    bool optional = t.optional();
    bool box = t.storageType() == StorageType::Box;

    if (resolveSelf && t.type() == TypeType::Self && typeContext.calleeType().resolveSelfOn_) {
        t = typeContext.calleeType();
    }

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

    if (t.canHaveGenericArguments()) {
        for (size_t i = 0; i < t.typeDefinition()->numberOfGenericArgumentsWithSuperArguments(); i++) {
            t.genericArguments_[i] = t.genericArguments_[i].resolveOn(typeContext);
        }
    }
    else if (t.type() == TypeType::Callable) {
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
    if (!to.typeDefinition()->ownGenericArgumentVariables().empty()) {
        for (size_t l = to.typeDefinition()->ownGenericArgumentVariables().size(),
             i = to.typeDefinition()->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
            if (!this->genericArguments_[i].identicalTo(to.genericArguments_[i], typeContext, ctargs)) {
                return false;
            }
        }
    }
    return true;
}

bool Type::compatibleTo(Type to, const TypeContext &ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (type() == TypeType::Error) {
        return to.type() == TypeType::Error && genericArguments()[0].identicalTo(to.genericArguments()[0], ct, ctargs)
                && genericArguments()[1].compatibleTo(to.genericArguments()[1], ct);
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

    if (to.type() == TypeType::Someobject && this->type() == TypeType::Class) {
        return true;
    }
    if (to.type() == TypeType::Error) {
        return compatibleTo(to.genericArguments()[1], ct);
    }
    if (this->type() == TypeType::Class && to.type() == TypeType::Class) {
        return this->eclass()->inheritsFrom(to.eclass()) && identicalGenericArguments(to, ct, ctargs);
    }
    if ((this->type() == TypeType::Protocol && to.type() == TypeType::Protocol) ||
        (this->type() == TypeType::ValueType && to.type() == TypeType::ValueType)) {
        return this->typeDefinition() == to.typeDefinition() && identicalGenericArguments(to, ct, ctargs);
    }
    if (type() == TypeType::MultiProtocol && to.type() == TypeType::MultiProtocol) {
        return std::equal(protocols().begin(), protocols().end(), to.protocols().begin(), to.protocols().end(),
                          [ct, ctargs](const Type &a, const Type &b) {
                              return a.compatibleTo(b, ct, ctargs);
                          });
    }
    if (to.type() == TypeType::MultiProtocol) {
        return std::all_of(to.protocols().begin(), to.protocols().end(), [this, ct](const Type &p) {
            return compatibleTo(p, ct);
        });
    }
    if (type() == TypeType::Class && to.type() == TypeType::Protocol) {
        for (Class *a = this->eclass(); a != nullptr; a = a->superclass()) {
            for (auto &protocol : a->protocols()) {
                if (protocol.resolveOn(*this).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                    return true;
                }
            }
        }
        return false;
    }
    if ((type() == TypeType::ValueType || type() == TypeType::Enum) && to.type() == TypeType::Protocol) {
        for (auto &protocol : typeDefinition()->protocols()) {
            if (protocol.resolveOn(*this).compatibleTo(to.resolveOn(ct), ct, ctargs)) {
                return true;
            }
        }
        return false;
    }
    if (this->type() == TypeType::Nothingness) {
        return to.optional() || to.type() == TypeType::Nothingness;
    }
    if (this->type() == TypeType::Enum && to.type() == TypeType::Enum) {
        return this->eenum() == to.eenum();
    }
    if ((this->type() == TypeType::GenericVariable && to.type() == TypeType::GenericVariable) ||
        (this->type() == TypeType::LocalGenericVariable && to.type() == TypeType::LocalGenericVariable)) {
        return (this->genericVariableIndex() == to.genericVariableIndex() &&
                this->typeDefinition() == to.typeDefinition()) ||
        this->resolveOnSuperArgumentsAndConstraints(ct)
        .compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (this->type() == TypeType::GenericVariable) {
        return this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (to.type() == TypeType::GenericVariable) {
        return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (this->type() == TypeType::LocalGenericVariable) {
        return ctargs != nullptr || this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (to.type() == TypeType::LocalGenericVariable) {
        if (ctargs != nullptr) {
            (*ctargs)[to.genericVariableIndex()].addType(*this, ct);
            return true;
        }
        return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
    }
    if (to.type() == TypeType::Self) {
        return this->type() == to.type();
    }
    if (this->type() == TypeType::Self) {
        return this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (this->type() == TypeType::Callable && to.type() == TypeType::Callable) {
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
            case TypeType::Self:
            case TypeType::Something:
            case TypeType::Someobject:
            case TypeType::Nothingness:
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

int Type::size() const {
    int basesize = 0;
    switch (storageType()) {
        case StorageType::SimpleOptional:
            basesize = 1;
        case StorageType::Simple:
            switch (type()) {
                case TypeType::ValueType:
                case TypeType::Enum:
                    return basesize + typeDefinition()->size();
                case TypeType::Callable:
                case TypeType::Class:
                case TypeType::Someobject:
                case TypeType::Self:
                    return basesize + 1;
                case TypeType::Error:
                    if (genericArguments()[1].storageType() == StorageType::SimpleOptional) {
                        return std::max(2, genericArguments()[1].size());
                    }
                    return std::max(1, genericArguments()[1].size()) + basesize;
                case TypeType::Nothingness:
                    return 1;
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
    if (type() == TypeType::Error) {
        return StorageType::SimpleOptional;
    }
    return optional() ? StorageType::SimpleOptional : StorageType::Simple;
}

EmojicodeInstruction Type::boxIdentifier() const {
    EmojicodeInstruction value;
    switch (type()) {
        case TypeType::ValueType:
        case TypeType::Enum:
            value = valueType()->boxIdFor(genericArguments_);
            break;
        case TypeType::Callable:
        case TypeType::Class:
        case TypeType::Someobject:
            value = T_OBJECT;
            break;
        case TypeType::Nothingness:
        case TypeType::Protocol:
        case TypeType::MultiProtocol:
        case TypeType::Something:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::Self:
        case TypeType::Error:
            return 0;  // This can only be executed in the case of a semantic error, return any value
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            throw std::logic_error("Box identifier for StorageExpectation/Extension");
    }
    return remotelyStored() ? value | REMOTE_MASK : value;
}

bool Type::requiresBox() const {
    switch (type()) {
        case TypeType::ValueType:
        case TypeType::Enum:
        case TypeType::Callable:
        case TypeType::Class:
        case TypeType::Someobject:
        case TypeType::Self:
        case TypeType::Nothingness:
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
        case TypeType::Self:
            return storageType() != StorageType::Simple;
        case TypeType::Nothingness:
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
            for (auto variable : valueType()->instanceScope().map()) {
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
        case TypeType::Self:
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
        case TypeType::Nothingness:
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            return;  // Can't be object pointer
    }
}

template void Type::objectVariableRecords(int, std::vector<ObjectVariableInformation>*) const;
template void Type::objectVariableRecords(int, std::vector<FunctionObjectVariableInformation>*,
                                          InstructionCount, InstructionCount) const;

// MARK: Type Visulisation

std::string Type::typePackage() const {
    switch (this->type()) {
        case TypeType::Class:
        case TypeType::ValueType:
        case TypeType::Protocol:
        case TypeType::Enum:
            return this->typeDefinition()->package()->name();
        case TypeType::Nothingness:
        case TypeType::Something:
        case TypeType::Someobject:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable:
        case TypeType::Callable:
        case TypeType::Self:
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
            string.append(type.typeDefinition()->name().utf8());
            break;
        case TypeType::MultiProtocol:
            string.append("🍱");
            for (auto &protocol : type.protocols()) {
                typeName(protocol, typeContext, string, package);
            }
            string.append("🍱");
            return;
        case TypeType::Nothingness:
            string.append("✨");
            return;
        case TypeType::Something:
            string.append("⚪️");
            return;
        case TypeType::Someobject:
            string.append("🔵");
            return;
        case TypeType::Self:
            string.append("🐕");
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
        case TypeType::GenericVariable: {
            if (typeContext.calleeType().type() == TypeType::Class) {
                Class *eclass = typeContext.calleeType().eclass();
                do {
                    for (auto it : eclass->ownGenericArgumentVariables()) {
                        if (it.second.genericVariableIndex() == type.genericVariableIndex()) {
                            string.append(it.first.utf8());
                            return;
                        }
                    }
                } while ((eclass = eclass->superclass()) != nullptr);
            }
            else if (typeContext.calleeType().canHaveGenericArguments()) {
                for (auto it : typeContext.calleeType().typeDefinition()->ownGenericArgumentVariables()) {
                    if (it.second.genericVariableIndex() == type.genericVariableIndex()) {
                        string.append(it.first.utf8());
                        return;
                    }
                }
            }

            string.append("T" + std::to_string(type.genericVariableIndex()) + "?");
            return;
        }
        case TypeType::LocalGenericVariable:
            if (typeContext.function() != nullptr) {
                for (auto it : typeContext.function()->genericArgumentVariables) {
                    if (it.second.genericVariableIndex() == type.genericVariableIndex()) {
                        string.append(it.first.utf8());
                        return;
                    }
                }
            }

            string.append("L" + std::to_string(type.genericVariableIndex()) + "?");
            return;
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            return;
    }

    if (type.canHaveGenericArguments()) {
        auto typeDef = type.typeDefinition();
        if (typeDef->ownGenericArgumentVariables().empty()) {
            return;
        }

        for (auto &argumentType : type.genericArguments()) {
            string.append("🐚");
            typeName(argumentType, typeContext, string, package);
        }
    }
}

std::string Type::toString(const TypeContext &typeContext, bool package) const {
    std::string string;
    typeName(*this, typeContext, string, package);
    return string;
}

}  // namespace EmojicodeCompiler
