//
//  Prettyprinter.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Prettyprinter.hpp"
#include "AST/ASTStatements.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Types/ValueType.hpp"
#include <algorithm>
#include <cstdio>

namespace EmojicodeCompiler {

void Prettyprinter::print() {
    auto first = true;
    for (auto &file : package_->files()) {
        stream_ = std::fstream(filePath(file.path_), std::ios_base::out);

        for (auto &recording : file.recordings_) {
            print(recording.get());
        }

        if (first) {
            first = false;
            stream_ << "🏁 ";
            if (package_->startFlagFunction()->returnType.type() != TypeType::NoReturn) {
                printReturnType(package_->startFlagFunction());
                stream_ << " ";
            }
            package_->startFlagFunction()->ast()->toCode(*this);
        }
    }
}

void Prettyprinter::printInterface(const std::string &out) {
    interface_ = true;
    stream_ = std::fstream(out, std::ios_base::out);

    printDocumentation(package_->documentation());
    stream_ << "🔮 " << package_->version().major << " " << package_->version().minor << "\n";
    offerNewLine();

    for (auto &file : package_->files()) {
        for (auto &recording : file.recordings_) {
            print(recording.get());
        }
    }
}

void Prettyprinter::print(RecordingPackage::Recording *recording) {
    if (auto import = dynamic_cast<RecordingPackage::Import *>(recording)) {
        refuseOffer() << "📦 " << import->package << " " << utf8(import->destNamespace) << "\n";
        offerNewLine();
    }
    if (auto type = dynamic_cast<RecordingPackage::RecordedType *>(recording)) {
        printTypeDef(type->type_);
    }
    if (auto include = dynamic_cast<RecordingPackage::Include *>(recording)) {
        refuseOffer() << "📜 🔤" << include->path_ << "🔤\n";
        offerNewLine();
    }
}

std::string Prettyprinter::filePath(const std::string &path) {
    std::rename(path.c_str(), (path + "_original").c_str());
    return path;
}

void Prettyprinter::printArguments(Function *function) {
    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        auto it = initializer->argumentsToVariables().begin();
        for (auto &arg : function->arguments) {
            if (it != initializer->argumentsToVariables().end() && arg.variableName == *it) {
                it++;
                stream_ << "🍼 ";
            }
            *this << utf8(arg.variableName) << " " << arg.type << " ";
        }
        return;
    }
    for (auto &arg : function->arguments) {
        *this << utf8(arg.variableName) << " " << arg.type << " ";
    }
}

void Prettyprinter::printReturnType(Function *function) {
    if (function->returnType.type() != TypeType::NoReturn) {
        *this << "➡️ " << function->returnType;
    }
}

void Prettyprinter::printDocumentation(const std::u32string &doc) {
    if (!doc.empty()) {
        indent() << "🌮" << utf8(doc) << "🌮\n";
    }
}

void Prettyprinter::printTypeDef(const Type &type) {
    auto typeDef = type.typeDefinition();

    printDocumentation(typeDef->documentation());

    if (typeDef->exported()) {
        stream_ << "🌍 ";
    }

    printTypeDefName(type);

    increaseIndent();
    *this << "🍇\n";

    if (auto protocol = type.protocol()) {
        for (auto method : protocol->methodList()) {
            print("❗️", method, false, true);
        }
        stream_ << "🍉\n\n";
        decreaseIndent();
        return;
    }
    if (auto enumeration = type.eenum()) {
        printEnumValues(enumeration);
    }

    printProtocolConformances(typeDef, TypeContext(type));
    printInstanceVariables(typeDef, TypeContext(type));
    printMethodsAndInitializers(typeDef);
    decreaseIndent();
    refuseOffer() << "🍉\n\n";
}

void Prettyprinter::printTypeDefName(const Type &type) {
    switch (type.type()) {
        case TypeType::Class:
            thisStream() << "🐇 ";
            break;
        case TypeType::ValueType:
            thisStream() << "🕊 ";
            break;
        case TypeType::Enum:
            thisStream() << "🦃 ";
            break;
        case TypeType::Protocol:
            thisStream() << "🐊 ";
            break;
        case TypeType::Extension:
            thisStream() << "🐋 ";
            break;
        default:
            break;
    }

    printNamespaceAccessor(type);
    stream_ << utf8(type.typeDefinition()->name());
    offerSpace();
    printGenericParameters(type.typeDefinition());

    if (auto klass = type.eclass()) {
        if (klass->superclass() != nullptr && type.type() != TypeType::Extension) {
            print(klass->superType(), TypeContext(type));
            stream_ << " ";
        }
    }
}

void Prettyprinter::printMethodsAndInitializers(TypeDefinition *typeDef) {
    for (auto init : typeDef->initializerList()) {
        print("🆕", init, true, true);
    }
    for (auto method : typeDef->methodList()) {
        print("❗️", method, true, false);
    }
    for (auto typeMethod : typeDef->typeMethodList()) {
        print("🐇❗️", typeMethod, true, true);
    }
}

void Prettyprinter::printProtocolConformances(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &protocol : typeDef->protocols()) {
        indent() << "🐊 ";
        print(protocol, typeContext);
        stream_ << "\n";
    }
    offerNewLineUnlessEmpty(typeDef->protocols());
}

void Prettyprinter::printInstanceVariables(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &ivar : typeDef->instanceVariables()) {
        indent() << "🍰 " << utf8(ivar.name) << " ";
        print(ivar.type, typeContext);
        stream_ << "\n";
    }
    offerNewLineUnlessEmpty(typeDef->instanceVariables());
}

void Prettyprinter::printEnumValues(Enum *enumeration) {
    auto values = std::vector<std::pair<std::u32string, std::u32string>>();
    std::transform(enumeration->values().begin(), enumeration->values().end(), std::back_inserter(values),
                   [](auto pair){ return std::make_pair(pair.first, pair.second.second); });
    std::sort(values.begin(), values.end(), [](auto &a, auto &b) { return a.first < b.first; });
    for (auto &value : values) {
        printDocumentation(value.second);
        indent() << "🔘" << utf8(value.first);
        stream_ << "\n";
    }
    offerNewLineUnlessEmpty(values);
}

void Prettyprinter::printFunctionAttributes(Function *function, bool noMutate) {
    if (function->deprecated()) {
        stream_ << "⚠️ ";
    }
    if (function->final()) {
        stream_ << "🔏 ";
    }
    if (function->overriding()) {
        stream_ << "✒️ ";
    }
    if (function->owningType().type() == TypeType::ValueType && function->mutating() && !noMutate) {
        stream_ << "🖍 ";
    }
}

void Prettyprinter::printFunctionAccessLevel(Function *function) {
    switch (function->accessLevel()) {
        case AccessLevel::Private:
            stream_ << "🔒";
            break;
        case AccessLevel::Protected:
            stream_ << "🔐";
            break;
        case AccessLevel::Public:
            break;
    }
}

void Prettyprinter::printClosure(Function *function) {
    stream_ << "🍇";
    printArguments(function);
    printReturnType(function);
    stream_ << "\n";
    function->ast()->innerToCode(*this);
    stream_ << "🍉\n";
}

void Prettyprinter::print(const char *key, Function *function, bool body, bool noMutate) {
    printDocumentation(function->documentation());
    typeContext_ = function->typeContext();

    indent();
    printFunctionAttributes(function, noMutate);
    printFunctionAccessLevel(function);

    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        if (initializer->required()) {
            stream_ << "🔑 ";
        }
    }

    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        stream_ << key;
        if (initializer->name().front() != E_NEW_SIGN) {
            stream_ << " " << utf8(function->name()) << " ";
        }
        if (initializer->errorProne()) {
            stream_ << "🚨";
            print(initializer->errorType(), typeContext_);
            stream_ << " ";
        }
    }
    else {
        if (!hasInstanceScope(function->functionType()) || operatorType(function->name()) == OperatorType::Invalid) {
            stream_ << key << " ";
        }

        stream_ << utf8(function->name()) << " ";
    }

    printArguments(function);
    printReturnType(function);
    if (body && !interface_) {
        function->ast()->toCode(*this);
        offerNewLine();
    }
    else if (!function->externalName().empty()) {
        stream_ << " 📻 🔤" << function->externalName() << "🔤\n";
    }
    else {
        stream_ << "\n";
    }
    typeContext_ = TypeContext();
}

void Prettyprinter::print(const Type &type, const TypeContext &typeContext) {
    printNamespaceAccessor(type);
    stream_ << type.toString(typeContext, false);
}

void Prettyprinter::printNamespaceAccessor(const Type &type) {
    auto ns = package_->findNamespace(type);
    if (!ns.empty()) {
        stream_ << "🔶" << utf8(ns);
    }
}

}  // namespace EmojicodeCompiler
