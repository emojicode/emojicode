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
#include "Lex/SourceManager.hpp"
#include "Package/Package.hpp"
#include "Parsing/OperatorHelper.hpp"
#include "Types/Class.hpp"
#include "Types/Enum.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/Type.hpp"
#include "Emojis.h"
#include <algorithm>
#include <cstdio>
#include <iostream>

namespace EmojicodeCompiler {

void Prettyprinter::printRecordings(const std::vector<std::unique_ptr<RecordingPackage::Recording>> &recordings) {
    for (auto &recording : recordings) {
        print(recording.get());
    }
}

void Prettyprinter::print() {
    auto first = true;
    for (auto &file : package_->files()) {
        stream_ = std::fstream(filePath(file.path_), std::ios_base::out); // .basic_ios::rdbuf(std::cout.rdbuf()); 

        printRecordings(file.recordings_);

        if (first) {
            first = false;
            stream_ << "🏁 ";
            if (package_->startFlagFunction()->returnType() != nullptr) {
                printReturnType(package_->startFlagFunction());
                stream_ << " ";
            }
            lastCommentQuery_ = package_->startFlagFunction()->position();
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

    printRecordings(package_->files().front().recordings_);
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
        if (interface_) {
            printRecordings(package_->files()[interfaceFileIndex++].recordings_);
        }
        else {
            refuseOffer() << "📜 🔤" << include->path_ << "🔤\n";
            offerNewLine();
        }
    }
}

std::string Prettyprinter::filePath(const std::string &path) {
    std::rename(path.c_str(), (path + "_original").c_str());
    return path;
}

void Prettyprinter::printArguments(Function *function) {
    if (auto initializer = dynamic_cast<Initializer *>(function)) {
        auto it = initializer->argumentsToVariables().begin();
        for (auto &arg : function->parameters()) {
            if (it != initializer->argumentsToVariables().end() && arg.name == *it) {
                it++;
                stream_ << "🍼 ";
            }
            *this << utf8(arg.name) << " " << arg.type << " ";
        }
        return;
    }
    for (auto &arg : function->parameters()) {
        *this << utf8(arg.name) << " " << arg.type << " ";
    }
}

void Prettyprinter::printReturnType(Function *function) {
    if (function->returnType() == nullptr) {
        return;
    }
    *this << "➡️ " << function->returnType();
}

void Prettyprinter::printDocumentation(const std::u32string &doc) {
    if (!doc.empty()) {
        indent() << "📗" << utf8(doc) << "📗\n";
    }
}

void Prettyprinter::printTypeDef(const Type &type) {
    auto typeDef = type.typeDefinition();

    printDocumentation(typeDef->documentation());

    if (typeDef->exported()) {
        stream_ << "🌍 ";
    }
    if (auto klass = type.klass()) {
        if (klass->foreign()) {
            stream_ << "📻 ";
        }
    }

    printTypeDefName(type);

    increaseIndent();
    *this << "🍇\n";

    if (auto protocol = type.protocol()) {
        for (auto method : protocol->methodList()) {
            print(method->isImperative() ? "❗️" : "❓️", method, false, true);
        }
        stream_ << "🍉\n\n";
        decreaseIndent();
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
    decreaseIndent();
    refuseOffer() << "🍉\n\n";
}

void Prettyprinter::printTypeDefName(const Type &type) {
    auto typeDef = type.typeDefinition();

    switch (type.unboxedType()) {
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

    stream_ << type.namespaceAccessor(package_) << utf8(typeDef->name());
    offerSpace();
    printGenericParameters(typeDef);

    if (auto klass = type.klass()) {
        if (klass->superType() != nullptr && type.type() != TypeType::Extension) {
            thisStream() << klass->superType() << " ";
        }
    }
    if (auto valueType = type.valueType()) {
        if (valueType->isPrimitive() && type.type() != TypeType::Enum) {
            thisStream() << "⚪️ ";
        }
    }
}

void Prettyprinter::printMethodsAndInitializers(TypeDefinition *typeDef) {
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

void Prettyprinter::printProtocolConformances(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &protocol : typeDef->protocols()) {
        indent() << "🐊 " << protocol << "\n";
    }
    offerNewLineUnlessEmpty(typeDef->protocols());
}

void Prettyprinter::printInstanceVariables(TypeDefinition *typeDef, const TypeContext &typeContext) {
    for (auto &ivar : typeDef->instanceVariables()) {
        indent() << "🖍🆕 " << utf8(ivar.name) << " " << ivar.type << "\n";
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
    if (function->unsafe()) {
        stream_ << "☣️ ";
    }
    if (function->owner()->type().type() == TypeType::ValueType && function->mutating() && !noMutate) {
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
    if (interface_ && function->accessLevel() == AccessLevel::Private) {
        return;
    }

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
            thisStream() << " " << utf8(function->name()) << " ";
        }
        if (initializer->errorProne()) {
            thisStream() << "🚨" << initializer->errorType() << " ";
        }
    }
    else {
        if (!hasInstanceScope(function->functionType()) || operatorType(function->name()) == OperatorType::Invalid) {
            stream_ << key << " ";
        }

        stream_ << utf8(function->name()) << " ";
    }

    printGenericParameters(function);
    printArguments(function);
    printReturnType(function);
    if (body && !interface_) {
        lastCommentQuery_ = function->position();
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
    stream_ << type.toString(typeContext, package_);
}

void Prettyprinter::printComments(const SourcePosition &p) {
    if (p.file == nullptr) {
        return;
    }
    p.file->findComments(lastCommentQuery_, p, [this, &p](const Token &comment) {
        if (whitespaceOffer_ == '\n') {
            if (comment.position().line >= p.line) {
                stream_ << whitespaceOffer_;
                whitespaceOffer_ = 0;
                indent();
            }
            else {
                stream_ << "  ";
            }
        }

        stream_ << (comment.type() == TokenType::MultilineComment ? "💭🔜" : "💭") << utf8(comment.value());
        if (comment.type() == TokenType::MultilineComment) stream_ << "🔚💭";
        else offerNewLine();
    });
    lastCommentQuery_ = p;
}

}  // namespace EmojicodeCompiler
