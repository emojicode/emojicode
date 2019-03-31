//
//  PrettyPrinter.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 23.06.18.
//

#include "PrettyPrinter.hpp"
#include "AST/ASTStatements.hpp"
#include "Emojis.h"
#include "Functions/Function.hpp"
#include "Functions/Initializer.hpp"
#include "Package/Package.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Lex/SourceManager.hpp"
#include "Scoping/Scope.hpp"
#include <algorithm>
#include <iostream>

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
            if (arg.memoryFlowType.isEscaping()) {
                prettyStream_ << "🎍🥡 ";
            }
            prettyStream_ << arg.name << " " << arg.type << " ";
        }
        return;
    }
    for (auto &arg : function->parameters()) {
        if (arg.memoryFlowType.isEscaping()) {
            prettyStream_ << "🎍🥡 ";
        }
        prettyStream_ << arg.name << " " << arg.type << " ";
    }
}

void PrettyPrinter::printClosure(Function *function, bool escaping) {
    prettyStream_ << "🍇";
    if (escaping) prettyStream_ << "🎍🥡 ";
    printArguments(function);
    printReturnType(function);
    printErrorType(function);
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
            print(moodEmoji(method->mood()), method, false, true);
        }
        prettyStream_ << "🍉\n\n";
        prettyStream_.decreaseIndent();
        return;
    }
    if (auto enumeration = type.enumeration()) {
        printEnumValues(enumeration);
    }

    auto context = TypeContext(type);
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
        default:
            break;
    }

    prettyStream_ << type.namespaceAccessor(package_) << typeDef->name();
    prettyStream_.offerSpace();
    printGenericParameters(typeDef);

    if (auto klass = type.klass()) {
        if (klass->superType() != nullptr) {
            prettyStream_ << klass->superType() << " ";
        }
    }
}

void PrettyPrinter::printMethodsAndInitializers(TypeDefinition *typeDef) {
    for (auto init : typeDef->initializerList()) {
        print("🆕", init, true, true);
    }
    for (auto method : typeDef->methodList()) {
        print(moodEmoji(method->mood()), method, true, false);
    }
    for (auto typeMethod : typeDef->typeMethodList()) {
        print(moodEmoji(typeMethod->mood()), typeMethod, true, true);
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
        if (interface_ && typeDef->instanceScope().getLocalVariable(ivar.name).inherited()) {
            continue;
        }
        prettyStream_.indent() << "🖍🆕 " << ivar.name << " " << ivar.type;
        if (ivar.expr != nullptr) {
            prettyStream_ << " ⬅️ " << ivar.expr;
        }
        prettyStream_ << "\n";
    }
    prettyStream_.offerNewLineUnlessEmpty(typeDef->instanceVariables());
}

void PrettyPrinter::printEnumValues(Enum *enumeration) {
    auto values = std::vector<std::pair<std::u32string, EnumValue>>();
    std::transform(enumeration->values().begin(), enumeration->values().end(), std::back_inserter(values),
                   [](auto pair){ return std::make_pair(pair.first, pair.second); });
    std::sort(values.begin(), values.end(), [](auto &a, auto &b) { return a.second.value < b.second.value; });
    for (auto &value : values) {
        printDocumentation(value.second.documentation);
        prettyStream_.indent() << "🔘" << value.first << "\n";
    }
    prettyStream_.offerNewLineUnlessEmpty(values);
}

void PrettyPrinter::printFunctionAttributes(Function *function, bool noMutate) {
    if (function->isInline()) {
        prettyStream_ << "🥯 ";
    }
    if (function->deprecated()) {
        prettyStream_ << "⚠️ ";
    }
    if (function->final()) {
        prettyStream_ << "🔏 ";
    }
    if (function->overriding()) {
        prettyStream_ << "✒️ ";
    }
    if (function->functionType() == FunctionType::ClassMethod ||
        (function->functionType() == FunctionType::Function &&
         dynamic_cast<ValueType*>(function->owner()) != nullptr)) {
        prettyStream_ << "🐇 ";
    }
    if (function->unsafe()) {
        prettyStream_ << "☣️ ";
    }
    if (function->owner()->type().type() == TypeType::ValueType && function->mutating() && !noMutate) {
        prettyStream_ << "🖍 ";
    }
    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        if (initializer->required()) {
            prettyStream_ << "🔑 ";
        }
    }
    if (!function->memoryFlowTypeForThis().isUnknown() && function->memoryFlowTypeForThis().isEscaping()) {
        prettyStream_ << "🎍🥡 ";
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
            prettyStream_ << "🔓";
            break;
        case AccessLevel::Default:
            break;
    }
}

void PrettyPrinter::printErrorType(Function *function) {
    if (function->errorType() != nullptr && !(function->errorType()->wasAnalysed() &&
                                              function->errorType()->type().type() == TypeType::NoReturn)) {
        prettyStream_ << "🚧" << function->errorType() << " ";
    }
}

void PrettyPrinter::print(const char *key, Function *function, bool body, bool noMutate) {
    if (function->isThunk()) {
        return;
    }
    printDocumentation(function->documentation());

    prettyStream_.withTypeContext(function->typeContext(), [&]() {
        prettyStream_.indent();
        printFunctionAttributes(function, noMutate);
        printFunctionAccessLevel(function);

        if (auto initializer = dynamic_cast<Initializer *>(function)) {
            prettyStream_ << key;
            if (initializer->name().front() != E_NEW_SIGN) {
                prettyStream_ << " " << function->name() << " ";
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
        printErrorType(function);

        if (!function->externalName().empty()) {
            prettyStream_ << " 📻 🔤" << function->externalName() << "🔤";
        }
        else if (body) {
            if (interface_) {
                if (function->isInline()) {
                    auto str = function->position().file->file();
                    auto code = str.substr(function->ast()->beginIndex(),
                                           function->ast()->endIndex() - function->ast()->beginIndex() + 1);
                    prettyStream_ << " 🍇\n";
                    prettyStream_.increaseIndent();
                    prettyStream_.indent() << code;
                    prettyStream_.decreaseIndent();
                }
            }
            else {
                prettyStream_.setLastCommentQueryPlace(function->position());
                function->ast()->toCode(prettyStream_);

            }
        }
        prettyStream_ << "\n";
    });
}

}  // namespace EmojicodeCompiler
