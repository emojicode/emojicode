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
        default:
            stream << "ty_";
            break;
    }
    mangleIdentifier(stream, type.typeDefinition()->name());
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

std::string mangleFunctionName(Function *function) {
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
    return stream.str();
}

std::string mangleTypeName(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    return stream.str();
}

}  // namespace EmojicodeCompiler
