//
//  Mangler.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 05/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Mangler.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Types/Class.hpp"
#include "Types/Type.hpp"
#include "Types/ValueType.hpp"
#include <sstream>

namespace EmojicodeCompiler {

void mangleIdentifier(std::stringstream &stream, const std::u32string &string) {
    bool first = true;
    for (auto ch : string) {
        if (first) {
            first = false;
        }
        else {
            stream << "_";
        }
        stream << std::hex << ch;
    }
}

void mangleTypeName(std::stringstream &stream, const Type &type) {
    stream << type.typePackage() << "_";
    switch (type.type()) {
        case TypeType::ValueType:
            stream << "vt_";
            break;
        case TypeType::Class:
            stream << "class_";
            break;
        case TypeType::Enum:
            stream << "enum_";
            break;
        case TypeType::Protocol:
            stream << "protocol_";
            break;
        default:
            stream << "ty_";
            break;
    }
    mangleIdentifier(stream, type.typeDefinition()->name());
}

void mangleGenericArguments(std::stringstream &stream, const std::map<size_t, Type> &genericArgs) {
    for (auto &pair : genericArgs) {
        stream << '$' << pair.first << '_';
        mangleTypeName(stream, pair.second);
    }
}

std::string mangleClassMetaName(Class *klass) {
    std::stringstream stream;
    stream << klass->package()->name() << "_class_meta_";
    mangleIdentifier(stream, klass->name());
    return stream.str();
}

std::string mangleValueTypeMetaName(const Type &type) {
    std::stringstream stream;
    stream << type.typePackage() << "_value_meta_";
    mangleIdentifier(stream, type.typeDefinition()->name());
    for (auto &arg : type.genericArguments()) {
        mangleTypeName(stream, arg);
    }
    return stream.str();
}

std::string mangleFunction(Function *function, const std::map<size_t, Type> &genericArgs) {
    std::stringstream stream;
    if (function->owningType().type() != TypeType::NoReturn) {
        mangleTypeName(stream, function->owningType());
        if (isFullyInitializedCheckRequired(function->functionType())) {
            stream << ".init";
        }
        stream << ".";
    }
    else {
        stream << "fn_";
    }

    mangleIdentifier(stream, function->name());
    if (!function->isImperative()) {
        stream << "_intrg";
    }
    mangleGenericArguments(stream, genericArgs);
    return stream.str();
}

std::string mangleTypeName(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    return stream.str();
}

std::string mangleProtocolConformance(const Type &type, const Type &protocol) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    stream << "_conformance_";
    mangleTypeName(stream, protocol);
    return stream.str();
}

}  // namespace EmojicodeCompiler
