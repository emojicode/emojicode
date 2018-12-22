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
#include "Generation/ReificationContext.hpp"
#include <sstream>

namespace EmojicodeCompiler {

void mangleTypeName(std::stringstream &stream, const Type &typeb);

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

void mangleGenericArguments(std::stringstream &stream, const std::map<size_t, Type> &genericArgs) {
    for (auto &pair : genericArgs) {
        stream << '$' << pair.first << '_';
        mangleTypeName(stream, pair.second);
    }
}

std::string mangleTypeDefinition(std::stringstream &stream, TypeDefinition *typeDef,
                                 const std::map<size_t, Type> &genericArgs) {
    stream << typeDef->package()->name() << ".";
    mangleIdentifier(stream, typeDef->name());
    mangleGenericArguments(stream, genericArgs);
    return stream.str();
}

void mangleTypeName(std::stringstream &stream, const Type &type) {
    stream << type.typePackage() << ".";
    switch (type.type()) {
        case TypeType::ValueType:
        case TypeType::Class:
        case TypeType::Enum:
        case TypeType::Protocol:
            mangleTypeDefinition(stream, type.typeDefinition(),
                                 type.typeDefinition()->reificationWrapperFor(type.genericArguments()).arguments);
        case TypeType::TypeAsValue:
            stream << "tyval_";
            break;
        case TypeType::Callable:
            stream << "callable_";
            for (auto &type : type.genericArguments()) {
                mangleTypeName(stream, type);
            }
            return;
        case TypeType::NoReturn:
            stream << "no_return_";
            return;
        case TypeType::Box:
            stream << "box_";
            return;
        default:
            stream << "ty_";
            break;
    }
}

std::string mangleTypeDefinition(TypeDefinition *typeDef, const Reification<TypeDefinitionReification> *reification) {
    std::stringstream stream;
    mangleTypeDefinition(stream, typeDef, reification->arguments);
    return stream.str();
}

std::string mangleClassInfoName(Class *klass) {
    std::stringstream stream;
    stream << klass->package()->name() << "_class_info_";
    mangleIdentifier(stream, klass->name());
    return stream.str();
}

std::string mangleFunction(Function *function, const ReificationContext &reificationContext) {
    std::stringstream stream;
    if (function->owner() != nullptr) {
        mangleTypeDefinition(stream, function->owner(), reificationContext.ownerReification()->arguments);
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
    mangleGenericArguments(stream, reificationContext.arguments());
    return stream.str();
}

std::string mangleTypeName(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
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

std::string mangleProtocolConformance(const Type &type, const Type &protocol) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    stream << ".conformances.";
    mangleTypeName(stream, protocol);
    return stream.str();
}

std::string mangleProtocolIdentifier(const Type &type) {
    std::stringstream stream;
    mangleTypeName(stream, type);
    stream << "_identifier";
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
