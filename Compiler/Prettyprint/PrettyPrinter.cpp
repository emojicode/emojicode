//
//  PrettyPrinter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 23.06.18.
//

#include "PrettyPrinter.hpp"
#include "AST/ASTStatements.hpp"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Emojis.h"
#include <algorithm>

namespace EmojicodeCompiler {

void PrettyPrinter::printRecordings(const std::vector<std::unique_ptr<RecordingPackage::Recording>> &recordings) {
    for (auto &recording : recordings) {
        print(recording.get());
    }
}

void PrettyPrinter::print() {
    auto first = true;
    for (auto &file : package_->files()) {
        prettyStream_.setOutPath(filePath(file.path_));

        printRecordings(file.recordings_);

        if (first) {
            first = false;
            prettyStream_ << "🏁 ";
            if (package_->startFlagFunction()->returnType() != nullptr) {
                printReturnType(package_->startFlagFunction());
                prettyStream_ << " ";
            }
            prettyStream_.setLastCommentQueryPlace(package_->startFlagFunction()->position());
            package_->startFlagFunction()->ast()->toCode(prettyStream_);
        }
    }
}

void PrettyPrinter::printInterface(const std::string &out) {
    interface_ = true;
    prettyStream_.setOutPath(out);

    if (!package_->documentation().empty()) {
        prettyStream_.indent() << "📘" << package_->documentation() << "📘\n";
    }
    prettyStream_.offerNewLine();

    printRecordings(package_->files().front().recordings_);
    printLinkHints();
}

void PrettyPrinter::printLinkHints() {
    if (!package_->linkHints().empty()) {
        prettyStream_.indent() << "🔗 ";
        for (auto &hint : package_->linkHints()) {
            prettyStream_ << "🔤" << hint << "🔤 ";
        }
        prettyStream_ << "🔗\n";
    }
}

void PrettyPrinter::print(RecordingPackage::Recording *recording) {
    if (auto import = dynamic_cast<RecordingPackage::Import *>(recording)) {
        prettyStream_.refuseOffer() << "📦 " << import->package << " " << import->destNamespace << "\n";
        prettyStream_.offerNewLine();
    }
    if (auto type = dynamic_cast<RecordingPackage::RecordedType *>(recording)) {
        printTypeDef(type->type_);
    }
    if (auto include = dynamic_cast<RecordingPackage::Include *>(recording)) {
        if (interface_) {
            printRecordings(package_->files()[interfaceFileIndex++].recordings_);
        }
        else {
            prettyStream_.refuseOffer() << "📜 🔤" << include->path_ << "🔤\n";
            prettyStream_.offerNewLine();
        }
    }
}

std::string PrettyPrinter::filePath(const std::string &path) {
    std::rename(path.c_str(), (path + "_original").c_str());
    return path;
}

void PrettyPrinter::printArguments(Function *function) {
    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        auto it = initializer->argumentsToVariables().begin();
        for (auto &arg : function->parameters()) {
            if (it != initializer->argumentsToVariables().end() && arg.name == *it) {
                it++;
                prettyStream_ << "🍼 ";
            }
            if (arg.memoryFlowType == MFType::Escaping) {
                prettyStream_ << "🛅 ";
            }
            prettyStream_ << arg.name << " " << arg.type << " ";
        }
        return;
    }
    for (auto &arg : function->parameters()) {
        if (arg.memoryFlowType == MFType::Escaping) {
            prettyStream_ << "🛅 ";
        }
        prettyStream_ << arg.name << " " << arg.type << " ";
    }
}

void PrettyPrinter::printClosure(Function *function) {
    prettyStream_ << "🍇";
    printArguments(function);
    printReturnType(function);
    prettyStream_ << "\n";
    function->ast()->innerToCode(prettyStream_);
    prettyStream_ << "🍉\n";
}

void PrettyPrinter::printReturnType(Function *function) {
    auto type = function->returnType();
    if (type == nullptr || (type->wasAnalysed() && type->type().type() == TypeType::NoReturn)) {
        return;
    }
    prettyStream_ << "➡️ " << type;
}

void PrettyPrinter::printDocumentation(const std::u32string &doc) {
    if (!doc.empty()) {
        prettyStream_.indent() << "📗" << doc << "📗\n";
    }
}

void PrettyPrinter::printTypeDef(const Type &type) {
    auto typeDef = type.typeDefinition();

    printDocumentation(typeDef->documentation());

    if (typeDef->exported()) {
        prettyStream_ << "🌍 ";
    }
    if (auto klass = type.klass()) {
        if (klass->foreign()) {
            prettyStream_ << "📻 ";
        }
    }
    if (auto valueType = type.valueType()) {
        if (valueType->isPrimitive() && type.type() != TypeType::Enum) {
            prettyStream_ << "📻 ";
        }
    }

    printTypeDefName(type);

    prettyStream_.increaseIndent();
    prettyStream_ << "🍇\n";

    if (auto protocol = type.protocol()) {
        for (auto method : protocol->methodList()) {
            print(method->isImperative() ? "❗️" : "❓️", method, false, true);
        }
        prettyStream_ << "🍉\n\n";
        prettyStream_.decreaseIndent();
        return;
    }
    if (auto enumeration = type.enumeration()) {
        printEnumValues(enumeration);
    }

    auto context = type.type() == TypeType::Extension ? TypeContext(dynamic_cast<Extension*>(typeDef)->extendedType()) :
    TypeContext(type);
    printProtocolConformances(typeDef, context);
    printInstanceVariables(typeDef, context);
    printMethodsAndInitializers(typeDef);

    if (!interface_) {
        if (auto klass = type.klass()) {
            auto block = klass->deinitializer()->ast();
            if (block != nullptr) {
                prettyStream_.indent() << "♻️" << block;
            }
        }
    }

    prettyStream_.decreaseIndent();
    prettyStream_.refuseOffer() << "🍉\n\n";
}

void PrettyPrinter::printTypeDefName(const Type &type) {
    auto typeDef = type.typeDefinition();

    switch (type.unboxedType()) {
        case TypeType::Class:
            prettyStream_ << "🐇 ";
            break;
        case TypeType::ValueType:
            prettyStream_ << "🕊 ";
            break;
        case TypeType::Enum:
            prettyStream_ << "🦃 ";
            break;
        case TypeType::Protocol:
            prettyStream_ << "🐊 ";
            break;
        case TypeType::Extension:
            prettyStream_ << "🐋 ";
            break;
        default:
            break;
    }

    prettyStream_ << type.namespaceAccessor(package_) << typeDef->name();
    prettyStream_.offerSpace();
    printGenericParameters(typeDef);

    if (auto klass = type.klass()) {
        if (klass->superType() != nullptr && type.type() != TypeType::Extension) {
            prettyStream_ << klass->superType() << " ";
        }
    }
}

void PrettyPrinter::printMethodsAndInitializers(TypeDefinition *typeDef) {
    for (auto init : typeDef->initializerList()) {
        print("🆕", init, true, true);
    }
    for (auto method : typeDef->methodList()) {
        print(method->isImperative() ? "❗️" : "❓️", method, true, false);
    }
    for (auto typeMethod : typeDef->typeMethodList()) {
        print(typeMethod->isImperative() ? "🐇❗️" : "🐇❓", typeMethod, true, true);
    }
}

void PrettyPrinter::printProtocolConformances(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &protocol : typeDef->protocols()) {
        prettyStream_.indent() << "🐊 " << protocol << "\n";
    }
    prettyStream_.offerNewLineUnlessEmpty(typeDef->protocols());
}

void PrettyPrinter::printInstanceVariables(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &ivar : typeDef->instanceVariables()) {
        prettyStream_.indent() << "🖍🆕 " << ivar.name << " " << ivar.type << "\n";
    }
    prettyStream_.offerNewLineUnlessEmpty(typeDef->instanceVariables());
}

void PrettyPrinter::printEnumValues(Enum *enumeration) {
    auto values = std::vector<std::pair<std::u32string, std::u32string>>();
    std::transform(enumeration->values().begin(), enumeration->values().end(), std::back_inserter(values),
                   [](auto pair){ return std::make_pair(pair.first, pair.second.second); });
    std::sort(values.begin(), values.end(), [](auto &a, auto &b) { return a.first < b.first; });
    for (auto &value : values) {
        printDocumentation(value.second);
        prettyStream_.indent() << "🔘" << value.first << "\n";
    }
    prettyStream_.offerNewLineUnlessEmpty(values);
}

void PrettyPrinter::printFunctionAttributes(Function *function, bool noMutate) {
    if (function->deprecated()) {
        prettyStream_ << "⚠️ ";
    }
    if (function->final()) {
        prettyStream_ << "🔏 ";
    }
    if (function->overriding()) {
        prettyStream_ << "✒️ ";
    }
    if (function->unsafe()) {
        prettyStream_ << "☣️ ";
    }
    if (function->owner()->type().type() == TypeType::ValueType && function->mutating() && !noMutate) {
        prettyStream_ << "🖍 ";
    }
    if (function->memoryFlowTypeForThis() == MFType::Escaping) {
        prettyStream_ << "🛅 ";
    }
}

void PrettyPrinter::printFunctionAccessLevel(Function *function) {
    switch (function->accessLevel()) {
        case AccessLevel::Private:
            prettyStream_ << "🔒";
            break;
        case AccessLevel::Protected:
            prettyStream_ << "🔐";
            break;
        case AccessLevel::Public:
            break;
    }
}

void PrettyPrinter::print(const char *key, Function *function, bool body, bool noMutate) {
    if (interface_ && function->accessLevel() == AccessLevel::Private) {
        return;
    }

    printDocumentation(function->documentation());

    prettyStream_.withTypeContext(function->typeContext(), [&]() {
        prettyStream_.indent();
        printFunctionAttributes(function, noMutate);
        printFunctionAccessLevel(function);

        if (auto initializer = dynamic_cast<Initializer *>(function)) {
            if (initializer->required()) {
                prettyStream_ << "🔑 ";
            }
        }

        if (auto initializer = dynamic_cast<Initializer *>(function)) {
            prettyStream_ << key;
            if (initializer->name().front() != E_NEW_SIGN) {
                prettyStream_ << " " << function->name() << " ";
            }
            if (initializer->errorProne()) {
                prettyStream_ << "🚨" << initializer->errorType() << " ";
            }
        }
        else {
            if (!hasInstanceScope(function->functionType()) || operatorType(function->name()) == OperatorType::Invalid) {
                prettyStream_ << key << " ";
            }

            prettyStream_ << function->name() << " ";
        }

        printGenericParameters(function);
        printArguments(function);
        printReturnType(function);
        if (body && !interface_) {
            prettyStream_.setLastCommentQueryPlace(function->position());
            function->ast()->toCode(prettyStream_);
            prettyStream_.offerNewLine();
        }
        else if (!function->externalName().empty()) {
            prettyStream_ << " 📻 🔤" << function->externalName() << "🔤\n";
        }
        else {
            prettyStream_ << "\n";
        }
    });
}

}  // namespace EmojicodeCompiler
