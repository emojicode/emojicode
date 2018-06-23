//
//  PackageReporter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 02/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "PackageReporter.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

void PackageReporter::reportDocumentation(const std::u32string &documentation) {
    if (!documentation.empty()) {
        writer_.Key("documentation");
        writer_.String(utf8(documentation));
    }
}

void PackageReporter::reportType(const Type &type, const TypeContext &tc) {
    writer_.StartObject();

    switch (type.type()) {
        case TypeType::Box:
            reportType(type.unboxed(), tc);
        case TypeType::Class:
        case TypeType::ValueType:
        case TypeType::Enum:
        case TypeType::Protocol: {
            writer_.Key("package");
            writer_.String(type.typePackage());

            writer_.Key("name");
            writer_.String(utf8(type.typeDefinition()->name()));

            reportGenericArguments(type, tc);
            break;
        }
        case TypeType::Callable:
            reportTypeTypeAndGenericArgs("Callable", type, tc);
            break;
        case TypeType::MultiProtocol:
            reportTypeTypeAndGenericArgs("MultiProtocol", type, tc);
            break;
        case TypeType::Optional:
            reportTypeTypeAndGenericArgs("Optional", type, tc);
            break;
        case TypeType::Error:
            reportTypeTypeAndGenericArgs("Error", type, tc);
            break;
        case TypeType::TypeAsValue:
            reportTypeTypeAndGenericArgs("TypeAsValue", type, tc);
            break;
        case TypeType::NoReturn:
            writer_.Key("type");
            writer_.String("NoReturn");
            break;
        case TypeType::Someobject:
        case TypeType::Something:
        case TypeType::GenericVariable:
        case TypeType::LocalGenericVariable: {
            auto typeName = type.toString(tc);

            writer_.Key("type");
            writer_.String("Literal");

            writer_.Key("name");
            writer_.String(typeName);
            break;
        }
        case TypeType::StorageExpectation:
        case TypeType::Extension:
            throw std::domain_error("Generating report for type StorageExpectation/Extension");
    }

    writer_.EndObject();
}

void PackageReporter::reportGenericArguments(const Type &type, const TypeContext &context) {
    writer_.Key("arguments");
    writer_.StartArray();
    for (auto &arg : type.genericArguments()) {
        reportType(arg, context);
    }
    writer_.EndArray();
}

void PackageReporter::reportTypeTypeAndGenericArgs(const char *typeTypeString, const Type &type,
                                                   const TypeContext &context) {
    writer_.Key("type");
    writer_.String(typeTypeString);
    reportGenericArguments(type, context);
}

void PackageReporter::reportFunction(Function *function, const TypeContext &tc) {
    if (function->accessLevel() == AccessLevel::Private) {
        return;
    }

    writer_.StartObject();
    writer_.Key("name");
    writer_.String(utf8(function->name()));

    writer_.Key("accessLevel");
    switch (function->accessLevel()) {
        case AccessLevel::Private:
            writer_.String("🔒");
            break;
        case AccessLevel::Protected:
            writer_.String("🔒");
            break;
        case AccessLevel::Public:
            writer_.String("🔓");
            break;
    }

    writer_.Key("unsafe");
    writer_.Bool(function->unsafe());
    writer_.Key("mutating");
    writer_.Bool(function->mutating());
    writer_.Key("final");
    writer_.Bool(function->final());

    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        if (initializer->errorProne()) {
            writer_.Key("errorType");
            reportType(initializer->errorType()->type(), tc);
        }
    }
    else {
        writer_.Key("returnType");
        reportType(function->returnType()->type(), tc);

        writer_.Key("mood");
        if (operatorType(function->name()) != OperatorType::Invalid) {
            writer_.String("");
        }
        else {
            writer_.String(function->isImperative() ? "❗️" : "❓");
        }
    }

    reportGenericParameters(function, tc);
    reportDocumentation(function->documentation());

    writer_.Key("parameters");
    writer_.StartArray();
    for (auto &param : function->parameters()) {
        writer_.StartObject();
        writer_.Key("type");
        reportType(param.type->type(), tc);
        writer_.Key("name");
        writer_.String(utf8(param.name));
        writer_.EndObject();
    }
    writer_.EndArray();

    writer_.EndObject();
}

void PackageReporter::reportExportedType(const Type &type) {
    writer_.StartObject();
    auto typeDef = type.typeDefinition();

    writer_.Key("type");
    switch (type.type()) {
        case TypeType::Class:
            writer_.String("Class");
            break;
        case TypeType::ValueType:
            writer_.String("Value Type");
            break;
        case TypeType::Enum:
            writer_.String("Enumeration");
            break;
        case TypeType::Protocol:
            writer_.String("Protocol");
            break;
        default:
            throw std::domain_error("Generating report for exported type which is not Class/Enum/ValueType/Protocol");
    }

    writer_.Key("name");
    writer_.String(utf8(typeDef->name()));

    writer_.Key("conformances");
    writer_.StartArray();
    for (auto &protocol : typeDef->protocols()) {
        reportType(protocol->type(), TypeContext(type));
    }
    writer_.EndArray();

    reportGenericParameters(typeDef, TypeContext(type));
    reportDocumentation(typeDef->documentation());

    writer_.Key("methods");
    printFunctions(typeDef->methodList(), type);

    writer_.Key("initializers");
    printFunctions(typeDef->initializerList(), type);

    writer_.Key("typeMethods");
    printFunctions(typeDef->typeMethodList(), type);

    if (type.type() == TypeType::Class) {
        auto klass = type.klass();
        if (klass->superclass() != nullptr) {
            writer_.Key("superclass");
            reportType(klass->superType()->type(), TypeContext(type));
        }

        writer_.Key("final");
        writer_.Bool(klass->final());
    }

    if (type.type() == TypeType::Enum) {
        auto enumeration = type.enumeration();
        writer_.Key("enumerationValues");
        writer_.StartArray();
        for (auto it : enumeration->values()) {
            writer_.StartObject();
            reportDocumentation(it.second.second);
            writer_.Key("value");
            writer_.String(utf8(it.first));
            writer_.EndObject();
        }
        writer_.EndArray();
    }
    writer_.EndObject();
}

PackageReporter::PackageReporter(Package *package)
        : wrapper_(std::cout), writer_(wrapper_), package_(package) {}

void PackageReporter::report() {
    writer_.StartObject();

    reportDocumentation(package_->documentation());

    writer_.Key("types");
    writer_.StartArray();
    for (auto &exportedType : package_->exportedTypes()) {
        reportExportedType(exportedType.type);
    }
    writer_.EndArray();

    writer_.EndObject();

    std::cout << std::endl;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
