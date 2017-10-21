//
//  Package.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "../Scoping/SemanticScoper.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Compiler.hpp"
#include "../CompilerError.hpp"
#include "../Generation/FunctionCodeGenerator.hpp"
#include "../Lex/Lexer.hpp"
#include "../Parsing/CompatibilityInfoProvider.hpp"
#include "../Parsing/DocumentParser.hpp"
#include "../Prettyprint/Prettyprinter.hpp"
#include "../Types/ValueType.hpp"
#include "../Types/Class.hpp"
#include "../Types/Protocol.hpp"
#include "../Types/Extension.hpp"
#include "Package.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>

namespace EmojicodeCompiler {

Class* getStandardClass(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), false, &type);
    if (type.type() != TypeType::Class) {
        throw CompilerError(errorPosition, "s package class ", utf8(name), " is missing.");
    }
    return type.eclass();
}

Protocol* getStandardProtocol(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), false, &type);
    if (type.type() != TypeType::Protocol) {
        throw CompilerError(errorPosition, "s package protocol ", utf8(name), " is missing.");
    }
    return type.protocol();
}

ValueType* getStandardValueType(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), false, &type);
    if (type.type() != TypeType::ValueType) {
        throw CompilerError(errorPosition, "s package value type ", utf8(name), " is missing.");
    }
    return type.valueType();
}

void loadStandard(Package *s, const SourcePosition &errorPosition) {
    // Order of the following calls is important as they will cause Box IDs to be assigned
    VT_BOOLEAN = getStandardValueType(std::u32string(1, E_OK_HAND_SIGN), s, errorPosition);
    VT_INTEGER = getStandardValueType(std::u32string(1, E_STEAM_LOCOMOTIVE), s, errorPosition);
    VT_DOUBLE = getStandardValueType(std::u32string(1, E_ROCKET), s, errorPosition);
    VT_SYMBOL = getStandardValueType(std::u32string(1, E_INPUT_SYMBOL_FOR_SYMBOLS), s, errorPosition);

    CL_STRING = getStandardClass(std::u32string(1, 0x1F521), s, errorPosition);
    CL_LIST = getStandardClass(std::u32string(1, 0x1F368), s, errorPosition);
    CL_DATA = getStandardClass(std::u32string(1, 0x1F4C7), s, errorPosition);
    CL_DICTIONARY = getStandardClass(std::u32string(1, 0x1F36F), s, errorPosition);

    PR_ENUMERATOR = getStandardProtocol(std::u32string(1, 0x1F361), s, errorPosition);
    PR_ENUMERATEABLE = getStandardProtocol(std::u32string(1, E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY), s, errorPosition);
}

Class* Package::add(std::unique_ptr<Class> &&cl) {
    classes_.emplace_back(std::move(cl));
    return classes_.back().get();
}

ValueType* Package::add(std::unique_ptr<ValueType> &&vt) {
    valueTypes_.emplace_back(std::move(vt));
    return valueTypes_.back().get();
}

Protocol* Package::add(std::unique_ptr<Protocol> &&protocol) {
    protocols_.emplace_back(std::move(protocol));
    return protocols_.back().get();
}

Function* Package::add(std::unique_ptr<Function> &&function) {
    functions_.emplace_back(std::move(function));
    return functions_.back().get();
}

Extension* Package::add(std::unique_ptr<Extension> &&extension) {
    extensions_.emplace_back(std::move(extension));
    return extensions_.back().get();
}

Package::Package(std::string name, std::string mainFilePath, Compiler *app)
    : name_(std::move(name)), mainFile_(std::move(mainFilePath)), compiler_(app) {}
Package::~Package() = default;

void Package::importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) {
    auto import = compiler_->loadPackage(name, p, this);

    importedPackages_.emplace_back(import);

    for (auto exported : import->exportedTypes_) {
        Type type = Type::noReturn();
        if (lookupRawType(TypeIdentifier(exported.name, ns, p), false, &type)) {
            throw CompilerError(p, "Package ", name , " could not be loaded into namespace ", utf8(ns),
                                " of package ", name_, ": ", utf8(exported.name),
                                " collides with a type of the same name in the same namespace.");
        }

        offerType(exported.type, exported.name, ns, false, p);
    }
}

void Package::includeDocument(const std::string &path, const std::string &relativePath) {
    DocumentParser(this, Lexer::lexFile(path), endsWith(path, "emojii")).parse();
}

void Package::parse() {
    if (name_ != "s") {
        importPackage("s", kDefaultNamespace, position());
    }
    
    includeDocument(mainFile_, "");

    if (!validVersion()) {
        throw CompilerError(position(), "Package ", name(), " does not provide a valid version.");
    }

    if (name_ == "s") {
        loadStandard(this, position());
    }
    finishedLoading_ = true;
}

void Package::analyse() {
    for (auto &extension : extensions_) {
        extension->prepareForSemanticAnalysis();
    }
    for (auto &vt : valueTypes_) {
        vt->prepareForSemanticAnalysis();
        enqueueFunctionsOfTypeDefinition(vt.get());
    }
    for (auto &eclass : classes()) {
        eclass->prepareForSemanticAnalysis();
        enqueueFunctionsOfTypeDefinition(eclass.get());
    }

    size_t protocolIndex = 0;
    for (auto &protocol : protocols_) {
        protocol->setIndex(protocolIndex++);
        protocol->prepareForSemanticAnalysis();
    }

    for (auto &function : functions()) {
        enqueueFunction(function.get());
    }

    while (!compiler_->analysisQueue.empty()) {
        try {
            auto function = compiler_->analysisQueue.front();
            SemanticAnalyser(function).analyse();
        }
        catch (CompilerError &ce) {
            compiler_->error(ce);
        }
        compiler_->analysisQueue.pop();
    }
}

void Package::enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef) const {
    typeDef->eachFunction([this](Function *function) {
        enqueueFunction(function);
    });
}

void Package::enqueueFunction(Function *function) const {
    if (!function->isExternal()) {
        compiler_->analysisQueue.emplace(function);
    }
}

Type Package::getRawType(const TypeIdentifier &typeId, bool optional) const {
    auto type = Type::noReturn();
    if (!lookupRawType(typeId, optional, &type)) {
        throw CompilerError(typeId.position, "Could not find a type named ", utf8(typeId.name), " in namespace ",
                            utf8(typeId.ns), ".");
    }
    return type;
}

bool Package::lookupRawType(const TypeIdentifier &typeId, bool optional, Type *type) const {
    if (typeId.ns == kDefaultNamespace && typeId.ns.size() == 1) {
        switch (typeId.name.front()) {
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    throw CompilerError(typeId.position, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                *type = Type::something();
                return true;
            case E_LARGE_BLUE_CIRCLE:
                *type = Type::someobject(optional);
                return true;
            case E_SPARKLES:
                throw CompilerError(typeId.position, "The Nothingness type may not be referenced to.");
        }
    }

    std::u32string key = typeId.ns + typeId.name;
    auto it = types_.find(key);

    if (it != types_.end()) {
        auto xtype = it->second;
        if (optional) {
            xtype.setOptional();
        }
        *type = xtype;
        return true;
    }
    return false;
}

void Package::exportType(Type t, std::u32string name, const SourcePosition &p) {
    if (finishedLoading()) {
        throw std::logic_error("The package did already finish loading. No more types can be exported.");
    }
    if (std::any_of(exportedTypes_.begin(), exportedTypes_.end(), [&name](auto &type) { return type.name == name; })) {
        throw CompilerError(p, "A type named ", utf8(name), " was already exported.");
    }
    exportedTypes_.emplace_back(t, name);
}

void Package::offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                        const SourcePosition &p) {
    std::u32string key = ns + name;
    types_.emplace(key, t);

    if (exportFromPkg) {
        exportType(t, name, p);
    }
}

std::u32string Package::findNamespace(const Type &type) {
    if (type.type() != TypeType::ValueType && type.type() != TypeType::Enum && type.type() != TypeType::Class &&
        type.type() != TypeType::Protocol) {
        return std::u32string();
    }
    auto key = kDefaultNamespace + type.typeDefinition()->name();
    auto it = types_.find(key);
    if (it != types_.end()) {
        return std::u32string();
    }
    for (auto &pair : types_) {
        if (pair.second.typeDefinition() == type.typeDefinition()) {
            auto cpy = pair.first;
            cpy.erase(cpy.size() - type.typeDefinition()->name().size(), type.typeDefinition()->name().size());
            return cpy;
        }
    }
    return std::u32string();
}

}  // namespace EmojicodeCompiler
