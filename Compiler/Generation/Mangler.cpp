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

void mangleTypeName(std::stringstream &stream, const Type &typeb) {
    auto type = typeb.unboxed();
    stream << type.typePackage() << ".";
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
        case TypeType::TypeAsValue:
            stream << "tyval_";
            break;
        case TypeType::Callable:
            stream << "callable_";
            for (auto it = type.parameters(); it < type.parametersEnd(); it++) {
                mangleTypeName(stream, *it);
            }
            stream << "__";
            mangleTypeName(stream, type.returnType());
            return;
        case TypeType::NoReturn:
            stream << "no_return";
            return;
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

std::string mangleClassInfoName(Class *klass) {
    std::stringstream stream;
    stream << klass->package()->name() << "_class_info_";
    mangleIdentifier(stream, klass->name());
    return stream.str();
}

std::string mangleFunction(Function *function, const std::map<size_t, Type> &genericArgs) {
    std::stringstream stream;
    if (function->owner() != nullptr) {
        mangleTypeName(stream, function->owner()->type());
        if (function->functionType() == FunctionType::Deinitializer) {
            stream << ".deinit";
            return stream.str();
        }
        if (function->functionType() == FunctionType::CopyRetainer) {
            stream << ".copy";
            return stream.str();
        }
        if (isFullyInitializedCheckRequired(function->functionType())) {
            stream << ".init";
        }
        else if (function->functionType() == FunctionType::ClassMethod ||
                function->functionType() == FunctionType::Function) {
            stream << ".type";
        }
        stream << ".";
    }
    else {
        stream << "fn_";
    }

    mangleIdentifier(stream, function->name());
    if (function->mood() == Mood::Interogative) {
        stream << "_intrg";
    }
    else if (function->mood() == Mood::Assignment) {
        stream << "_assign";
    }
    mangleGenericArguments(stream, genericArgs);
    return stream.str();
}

std::string mangleBoxRetain(const Type &type) {
    return mangleTypeName(type) + ".boxRetain";
}

std::string mangleBoxRelease(const Type &type) {
    return mangleTypeName(type) + ".boxRelease";
}

std::string mangleBoxInfoName(const Type &type) {
    return mangleTypeName(type) + ".boxInfo";
}

std::string mangleTypeName(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    return stream.str();
}

std::string mangleProtocolConformance(const Type &type, const Type &protocol) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    stream << ".conformances.";
    mangleTypeName(stream, protocol);
    return stream.str();
}

std::string mangleProtocolRunTimeTypeInfo(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    stream << "_rtti";
    return stream.str();
}

std::string mangleMultiprotocolConformance(const Type &multi, const Type &conformer) {
    std::stringstream stream;
    mangleTypeName(stream, conformer);
    stream << "_multi";
    for (auto &protocol : multi.protocols()) {
        mangleTypeName(stream, protocol);
    }
    return stream.str();
}

}  // namespace EmojicodeCompiler
