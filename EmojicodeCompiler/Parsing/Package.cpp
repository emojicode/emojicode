//
//  Package.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "../Scoping/SemanticScoper.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../CompilerError.hpp"
#include "../Generation/FnCodeGenerator.hpp"
#include "../Types/ValueType.hpp"
#include "Package.hpp"
#include "PackageParser.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <string>

namespace EmojicodeCompiler {

Class* getStandardClass(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::Class) {
        throw CompilerError(errorPosition, "s package class %s is missing.", name.utf8().c_str());
    }
    return type.eclass();
}

Protocol* getStandardProtocol(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::Protocol) {
        throw CompilerError(errorPosition, "s package protocol %s is missing.", name.utf8().c_str());
    }
    return type.protocol();
}

ValueType* getStandardValueType(const EmojicodeString &name, Package *_, const SourcePosition &errorPosition,
                                unsigned int boxId) {
    Type type = Type::nothingness();
    _->fetchRawType(name, globalNamespace, false, errorPosition, &type);
    if (type.type() != TypeContent::ValueType) {
        throw CompilerError(errorPosition, "s package value type %s is missing.", name.utf8().c_str());
    }
    if (type.boxIdentifier() != boxId) {
        throw CompilerError(errorPosition, "s package value type %s has improper box id.", name.utf8().c_str());
    }
    return type.valueType();
}

void loadStandard(Package *s, const SourcePosition &errorPosition) {
    // Order of the following calls is important as they will cause Box IDs to be assigned
    VT_BOOLEAN = getStandardValueType(EmojicodeString(E_OK_HAND_SIGN), s, errorPosition, T_BOOLEAN);
    VT_INTEGER = getStandardValueType(EmojicodeString(E_STEAM_LOCOMOTIVE), s, errorPosition, T_INTEGER);
    VT_DOUBLE = getStandardValueType(EmojicodeString(E_ROCKET), s, errorPosition, T_DOUBLE);
    VT_SYMBOL = getStandardValueType(EmojicodeString(E_INPUT_SYMBOL_FOR_SYMBOLS), s, errorPosition, T_SYMBOL);

    CL_STRING = getStandardClass(EmojicodeString(0x1F521), s, errorPosition);
    CL_LIST = getStandardClass(EmojicodeString(0x1F368), s, errorPosition);
    CL_DATA = getStandardClass(EmojicodeString(0x1F4C7), s, errorPosition);
    CL_DICTIONARY = getStandardClass(EmojicodeString(0x1F36F), s, errorPosition);

    PR_ENUMERATOR = getStandardProtocol(EmojicodeString(0x1F361), s, errorPosition);
    PR_ENUMERATEABLE = getStandardProtocol(EmojicodeString(E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY), s, errorPosition);
}

std::vector<Package *> Package::packagesLoadingOrder_;
std::map<std::string, Package *> Package::packages_;

Package* Package::loadPackage(const std::string &name, const EmojicodeString &ns, const SourcePosition &p) {
    Package *package = findPackage(name);

    if (package != nullptr) {
        if (!package->finishedLoading()) {
            throw CompilerError(p, "Circular dependency detected: %s (loaded first) and %s depend on each other.",
                                this->name().c_str(), name.c_str());
        }
    }
    else {
        auto path = packageDirectory + "/" + name + "/header.emojic";

        package = new Package(name, p);

        if (name != "s") {
            package->loadPackage("s", globalNamespace, p);
        }

        package->parse(path);
    }

    package->loadInto(this, ns, p);
    return package;
}

void Package::parse(const std::string &path) {
    packages_.emplace(name(), this);

    PackageParser(this, lex(path)).parse();

    if (!validVersion()) {
        throw CompilerError(SourcePosition(0, 0, path), "Package %s does not provide a valid version.",
                                     name().c_str());
    }

    if (name_ == "s") {
        loadStandard(this, position_);
        requiresNativeBinary_ = false;
    }

    packagesLoadingOrder_.push_back(this);

    for (auto vt : valueTypes_) {
        vt->prepareForSemanticAnalysis();
    }
    for (auto eclass : classes()) {
        eclass->prepareForSemanticAnalysis();
        enqueueFunctionsOfTypeDefinition(eclass);
    }
    for (auto &extension : extensions_) {
        extension.prepareForSemanticAnalysis();
        enqueueFunctionsOfTypeDefinition(&extension);
    }

    for (auto function : functions()) {
        enqueFunction(function);
    }

    while (!Function::analysisQueue.empty()) {
        try {
            auto function = Function::analysisQueue.front();
            SemanticAnalyser(function).analyse();
        }
        catch (CompilerError &ce) {
            printError(ce);
        }
        Function::analysisQueue.pop();
    }

    finishedLoading_ = true;
}

void Package::enqueueFunctionsOfTypeDefinition(TypeDefinition *typeDef) {
    typeDef->eachFunction([this](Function *function) {
        enqueFunction(function);
    });
}

void Package::enqueFunction(Function *function) {
    if (!function->isNative()) {
        Function::analysisQueue.emplace(function);
    }
}

Package* Package::findPackage(const std::string &name) {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second : nullptr;
}

bool Package::fetchRawType(TypeIdentifier ptn, bool optional, Type *type) {
    return fetchRawType(ptn.name, ptn.ns, optional, ptn.token.position(), type);
}

bool Package::fetchRawType(const EmojicodeString &name, const EmojicodeString &ns, bool optional,
                           const SourcePosition &p, Type *type) {
    if (ns == globalNamespace && ns.size() == 1) {
        switch (name.front()) {
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    throw CompilerError(p, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                *type = Type::something();
                return true;
            case E_LARGE_BLUE_CIRCLE:
                *type = Type::someobject(optional);
                return true;
            case E_SPARKLES:
                throw CompilerError(p, "The Nothingness type may not be referenced to.");
        }
    }

    EmojicodeString key = EmojicodeString(ns);
    key.append(name);
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

void Package::exportType(Type t, EmojicodeString name, const SourcePosition &p) {
    if (finishedLoading()) {
        throw std::logic_error("The package did already finish loading. No more types can be exported.");
    }
    if (std::any_of(exportedTypes_.begin(), exportedTypes_.end(), [&name](auto &type) { return type.name == name; })) {
        throw CompilerError(p, "A type named %s was already exported.", name.utf8().c_str());
    }
    exportedTypes_.emplace_back(t, name);
}

void Package::registerType(Type t, const EmojicodeString &name, const EmojicodeString &ns, bool exportFromPkg,
                           const SourcePosition &p) {
    EmojicodeString key = EmojicodeString(ns);
    key.append(name);
    types_.emplace(key, t);

    if (exportFromPkg) {
        exportType(t, name, p);
    }
}

void Package::loadInto(Package *destinationPackage, const EmojicodeString &ns, const SourcePosition &p) const {
    for (auto exported : exportedTypes_) {
        Type type = Type::nothingness();
        if (destinationPackage->fetchRawType(exported.name, ns, false, p, &type)) {
            throw CompilerError(p, "Package %s could not be loaded into namespace %s of package %s: %s collides with a type of the same name in the same namespace.", name().c_str(), ns.utf8().c_str(), destinationPackage->name().c_str(),
                                         exported.name.utf8().c_str());
        }

        destinationPackage->registerType(exported.type, exported.name, ns, false, p);
    }
}

}  // namespace EmojicodeCompiler
