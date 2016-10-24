//
//  Type.c
//  Emojicode
//
//  Created by Theo Weidmann on 04.03.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <vector>
#include "utf8.h"
#include "Class.hpp"
#include "Function.hpp"
#include "EmojicodeCompiler.hpp"
#include "Enum.hpp"
#include "Protocol.hpp"
#include "TypeContext.hpp"
#include "ValueType.hpp"

//MARK: Globals
/* Very important one time declarations */

Class *CL_STRING;
Class *CL_LIST;
Class *CL_ERROR;
Class *CL_DATA;
Class *CL_DICTIONARY;
Protocol *PR_ENUMERATEABLE;
Protocol *PR_ENUMERATOR;
Class *CL_RANGE;
ValueType *VT_BOOLEAN;
ValueType *VT_SYMBOL;
ValueType *VT_INTEGER;
ValueType *VT_DOUBLE;

Type::Type(Protocol *p, bool o) : typeDefinition_(p), typeContent_(TypeContent::Protocol), optional_(o)  {
}

Type::Type(Enum *e, bool o) : typeDefinition_(e), typeContent_(TypeContent::Enum), optional_(o) {
}

Type::Type(ValueType *v, bool o) : typeDefinition_(v), typeContent_(TypeContent::ValueType), optional_(o) {
}

Type::Type(Class *c, bool o) : typeDefinition_(c), typeContent_(TypeContent::Class), optional_(o) {
    for (int i = 0; i < c->numberOfGenericArgumentsWithSuperArguments(); i++) {
        genericArguments.push_back(Type(TypeContent::Reference, false, i, c));
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
    return static_cast<TypeDefinition *>(typeDefinition_);
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

bool Type::allowsMetaType() {
    return type() == TypeContent::Class || type() == TypeContent::Enum || type() == TypeContent::ValueType;
}

Type Type::resolveReferenceToBaseReferenceOnSuperArguments(TypeContext typeContext) const {
    TypeDefinitionFunctional *c = typeContext.calleeType().typeDefinitionFunctional();
    Type t = *this;
    
    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->numberOfOwnGenericArguments();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeContent::Reference && c->canBeUsedToResolve(c) && t.reference < maxReferenceForSuper) {
        Type tn = c->superGenericArguments()[t.reference];
        if (tn.type() != TypeContent::Reference) {
            break;
        }
    }
    return t;
}

Type Type::resolveOnSuperArgumentsAndConstraints(TypeContext typeContext, bool resolveSelf) const {
    if (typeContext.calleeType().type() == TypeContent::Nothingness) return *this;
    TypeDefinitionFunctional *c = typeContext.calleeType().typeDefinitionFunctional();
    Type t = *this;
    bool optional = t.optional();
    
    if (resolveSelf && t.type() == TypeContent::Self) {
        t = typeContext.calleeType();
    }
    
    auto maxReferenceForSuper = c->numberOfGenericArgumentsWithSuperArguments() - c->numberOfOwnGenericArguments();
    // Try to resolve on the generic arguments to the superclass.
    while (t.type() == TypeContent::Reference && t.reference < maxReferenceForSuper) {
        t = c->superGenericArguments()[t.reference];
    }
    while (t.type() == TypeContent::LocalReference) {
        t = typeContext.function()->genericArgumentConstraints[t.reference];
    }
    while (t.type() == TypeContent::Reference) {
        t = typeContext.calleeType().typeDefinitionFunctional()->genericArgumentConstraints()[t.reference];
    }
    
    if (optional) t.setOptional();
    return t;
}

Type Type::resolveOn(TypeContext typeContext, bool resolveSelf) const {
    Type t = *this;
    bool optional = t.optional();
    
    if (resolveSelf && t.type() == TypeContent::Self && typeContext.calleeType().resolveSelfOn_) {
        t = typeContext.calleeType();
    }
    
    while (t.type() == TypeContent::LocalReference && typeContext.functionGenericArguments()) {
        t = (*typeContext.functionGenericArguments())[t.reference];
    }
    
    if (typeContext.calleeType().canHaveGenericArguments()) {
        while (t.type() == TypeContent::Reference &&
               typeContext.calleeType().typeDefinitionFunctional()->canBeUsedToResolve(t.resolutionConstraint_)) {
            Type tn = typeContext.calleeType().genericArguments[t.reference];
            if (tn.type() == TypeContent::Reference && tn.reference == t.reference
                && tn.resolutionConstraint_ == t.resolutionConstraint_) {
                break;
            }
            t = tn;
        }
    }
    
    if (optional) t.setOptional();
    
    if (t.canHaveGenericArguments()) {
        for (int i = 0; i < t.typeDefinitionFunctional()->numberOfGenericArgumentsWithSuperArguments(); i++) {
            t.genericArguments[i] = t.genericArguments[i].resolveOn(typeContext);
        }
    }
    else if (t.type() == TypeContent::Callable) {
        for (int i = 0; i < t.genericArguments.size(); i++) {
            t.genericArguments[i] = t.genericArguments[i].resolveOn(typeContext);
        }
    }
    
    return t;
}

bool Type::identicalGenericArguments(Type to, TypeContext ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (to.typeDefinitionFunctional()->numberOfOwnGenericArguments()) {
        for (int l = to.typeDefinitionFunctional()->numberOfOwnGenericArguments(), i = to.typeDefinitionFunctional()->numberOfGenericArgumentsWithSuperArguments() - l; i < l; i++) {
            if (!this->genericArguments[i].identicalTo(to.genericArguments[i], ct, ctargs)) {
                return false;
            }
        }
    }
    return true;
}

bool Type::compatibleTo(Type to, TypeContext ct, std::vector<CommonTypeFinder> *ctargs) const {
    if (to.type() == TypeContent::Something) {
        return true;
    }
    if (to.meta_ != meta_) {
        return false;
    }
    if (this->optional() && !to.optional()) {
        return false;
    }
    
    if (to.type() == TypeContent::Someobject && this->type() == TypeContent::Class ||
        this->type() == TypeContent::Protocol || this->type() == TypeContent::Someobject) {
        return true;
    }
    if (this->type() == TypeContent::Class && to.type() == TypeContent::Class) {
        return this->eclass()->inheritsFrom(to.eclass()) && identicalGenericArguments(to, ct, ctargs);
    }
    if ((this->type() == TypeContent::Protocol && to.type() == TypeContent::Protocol) ||
        (this->type() == TypeContent::ValueType && to.type() == TypeContent::ValueType)) {
        return this->typeDefinition() == to.typeDefinition() && identicalGenericArguments(to, ct, ctargs);
    }
    if (this->type() == TypeContent::Class && to.type() == TypeContent::Protocol) {
        for (Class *a = this->eclass(); a != nullptr; a = a->superclass()) {
            for (auto protocol : a->protocols()) {
                if (protocol.resolveOn(*this).compatibleTo(to, ct, ctargs)) return true;
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
        return this->reference == to.reference && this->resolutionConstraint_ == to.resolutionConstraint_ ||
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
        return ctargs || this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (to.type() == TypeContent::LocalReference) {
        if (ctargs) {
            (*ctargs)[to.reference].addType(*this, ct);
            return true;
        }
        else {
            return this->compatibleTo(to.resolveOnSuperArgumentsAndConstraints(ct), ct, ctargs);
        }
    }
    if (to.type() == TypeContent::Self) {
        return this->type() == to.type();
    }
    if (this->type() == TypeContent::Self) {
        return this->resolveOnSuperArgumentsAndConstraints(ct).compatibleTo(to, ct, ctargs);
    }
    if (this->type() == TypeContent::Callable && to.type() == TypeContent::Callable) {
        if (this->genericArguments[0].compatibleTo(to.genericArguments[0], ct, ctargs)
            && to.genericArguments.size() == this->genericArguments.size()) {
            for (int i = 1; i < to.genericArguments.size(); i++) {
                if (!to.genericArguments[i].compatibleTo(this->genericArguments[i], ct, ctargs)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    return this->type() == to.type();
}

bool Type::identicalTo(Type to, TypeContext tc, std::vector<CommonTypeFinder> *ctargs) const {
    if (ctargs && to.type() == TypeContent::LocalReference) {
        (*ctargs)[to.reference].addType(*this, tc);
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
                return to.genericArguments.size() == this->genericArguments.size()
                        && identicalGenericArguments(to, tc, ctargs);
            case TypeContent::Enum:
                return eenum() == to.eenum();
            case TypeContent::Reference:
            case TypeContent::LocalReference:
                return resolveReferenceToBaseReferenceOnSuperArguments(tc).reference ==
                        to.resolveReferenceToBaseReferenceOnSuperArguments(tc).reference;
            case TypeContent::Self:
            case TypeContent::Something:
            case TypeContent::Someobject:
            case TypeContent::Nothingness:
                return true;
        }
    }
    return false;
}

//MARK: Type Visulisation

const char* Type::typePackage() {
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
            return "";
    }
}

void stringAppendEc(EmojicodeChar c, std::string &string) {
    ecCharToCharStack(c, sc);
    string.append(sc);
}

void Type::typeName(Type type, TypeContext typeContext, bool includePackageAndOptional, std::string &string) const {
    if (type.meta_) {
        stringAppendEc(E_WHITE_SQUARE_BUTTON, string);
    }
    
    if (includePackageAndOptional) {
        if (type.optional()) {
            stringAppendEc(E_CANDY, string);
        }
        
        string.append(type.typePackage());
    }
    
    switch (type.type()) {
        case TypeContent::Class:
        case TypeContent::Protocol:
        case TypeContent::Enum:
        case TypeContent::ValueType:
            stringAppendEc(type.typeDefinition()->name(), string);
            break;
        case TypeContent::Nothingness:
            stringAppendEc(E_SPARKLES, string);
            return;
        case TypeContent::Something:
            stringAppendEc(E_MEDIUM_WHITE_CIRCLE, string);
            return;
        case TypeContent::Someobject:
            stringAppendEc(E_LARGE_BLUE_CIRCLE, string);
            return;
        case TypeContent::Self:
            stringAppendEc(E_DOG, string);
            return;
        case TypeContent::Callable:
            stringAppendEc(E_GRAPES, string);
            
            for (int i = 1; i < type.genericArguments.size(); i++) {
                typeName(type.genericArguments[i], typeContext, includePackageAndOptional, string);
            }
            
            stringAppendEc(E_RIGHTWARDS_ARROW, string);
            stringAppendEc(0xFE0F, string);
            typeName(type.genericArguments[0], typeContext, includePackageAndOptional, string);
            stringAppendEc(E_WATERMELON, string);
            return;
        case TypeContent::Reference: {
            if (typeContext.calleeType().type() == TypeContent::Class) {
                Class *eclass = typeContext.calleeType().eclass();
                do {
                    for (auto it : eclass->ownGenericArgumentVariables()) {
                        if (it.second.reference == type.reference) {
                            string.append(it.first.utf8CString());
                            return;
                        }
                    }
                } while ((eclass = eclass->superclass()));
            }
            else if (typeContext.calleeType().canHaveGenericArguments()) {
                for (auto it : typeContext.calleeType().typeDefinitionFunctional()->ownGenericArgumentVariables()) {
                    if (it.second.reference == type.reference) {
                        string.append(it.first.utf8CString());
                        return;
                    }
                }
            }
            
            stringAppendEc('T', string);
            stringAppendEc('0' + type.reference, string);
            return;
        }
        case TypeContent::LocalReference:
            if (typeContext.function()) {
                for (auto it : typeContext.function()->genericArgumentVariables) {
                    if (it.second.reference == type.reference) {
                        string.append(it.first.utf8CString());
                        return;
                    }
                }
            }
            
            stringAppendEc('L', string);
            stringAppendEc('0' + type.reference, string);
            return;
    }
    
    if (typeContext.calleeType().type() != TypeContent::Nothingness && type.canHaveGenericArguments()) {
        auto typeDef = type.typeDefinitionFunctional();
        int offset = typeDef->numberOfGenericArgumentsWithSuperArguments() - typeDef->numberOfOwnGenericArguments();
        for (int i = 0, l = typeDef->numberOfOwnGenericArguments(); i < l; i++) {
            stringAppendEc(E_SPIRAL_SHELL, string);
            typeName(type.genericArguments[offset + i], typeContext, includePackageAndOptional, string);
        }
    }
}

std::string Type::toString(TypeContext typeContext, bool includeNsAndOptional) const {
    std::string string;
    typeName(*this, typeContext, includeNsAndOptional, string);
    return string;
}
