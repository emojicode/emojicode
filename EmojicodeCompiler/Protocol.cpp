//
//  Protocol.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Protocol.hpp"
#include "Procedure.hpp"
#include "utf8.h"
#include "TypeContext.hpp"

uint_fast16_t Protocol::nextIndex = 0;

Protocol::Protocol(EmojicodeChar name, Package *pkg, const EmojicodeString &string)
    : TypeDefinitionWithGenerics(name, pkg, string) {
    if (nextIndex == UINT16_MAX) {
        throw CompilerErrorException(SourcePosition(0, 0, "blalblalb"), "You exceeded the limit of 65,536 protocols."); //TODO: ob
    }
    index = nextIndex++;
}

Method* Protocol::lookupMethod(EmojicodeChar name) {
    auto it = methods_.find(name);
    return it != methods_.end() ? it->second : nullptr;
}

Method* Protocol::getMethod(const Token &token, Type type, TypeContext typeContext) {
    auto method = lookupMethod(token.value[0]);
    if (method == nullptr) {
        auto eclass = type.toString(typeContext, true);
        ecCharToCharStack(token.value[0], method);
        throw CompilerErrorException(token, "%s has no method %s", eclass.c_str(), method);
    }
    return method;
}

void Protocol::addMethod(Method *method) {
    duplicateDeclarationCheck(method, methods_, method->position());
    method->vti = methodList_.size();
    methods_[method->name] = method;
    methodList_.push_back(method);
}

bool Protocol::canBeUsedToResolve(TypeDefinitionWithGenerics *resolutionConstraint) {
    return resolutionConstraint == this;
}