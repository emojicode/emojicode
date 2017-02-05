//
//  Package.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include <cstring>
#include <list>
#include <string>
#include <map>
#include "Package.hpp"
#include "PackageParser.hpp"
#include "CompilerError.hpp"

std::vector<Package *> Package::packagesLoadingOrder_;
std::map<std::string, Package *> Package::packages_;

Package* Package::loadPackage(std::string name, EmojicodeString ns, SourcePosition errorPosition) {
    Package *package = findPackage(name);

    if (package) {
        if (!package->finishedLoading()) {
            throw CompilerError(errorPosition,
                                         "Circular dependency detected: %s (loaded first) and %s depend on each other.",
                                         this->name().c_str(), name.c_str());
        }
    }
    else {
        auto path = packageDirectory + "/" + name + "/header.emojic";

        package = new Package(name, errorPosition);

        if (name != "s") {
            package->loadPackage("s", globalNamespace, errorPosition);
        }
        package->parse(path);
    }

    package->loadInto(this, ns, errorPosition);
    return package;
}

void Package::parse(std::string path) {
    packages_.emplace(name(), this);

    PackageParser(this, lex(path)).parse();

    if (!validVersion()) {
        throw CompilerError(SourcePosition(0, 0, path), "Package %s does not provide a valid version.",
                                     name().c_str());
    }

    packagesLoadingOrder_.push_back(this);

    finishedLoading_ = true;
}

Package* Package::findPackage(const std::string &name) {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second : nullptr;
}

bool Package::fetchRawType(ParsedTypeName ptn, Type *type) {
    return fetchRawType(ptn.name, ptn.ns, ptn.optional, ptn.token.position(), type);
}

bool Package::fetchRawType(EmojicodeString name, EmojicodeString ns, bool optional, SourcePosition ep, Type *type) {
    if (ns == globalNamespace && ns.size() == 1) {
        switch (name.front()) {
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    throw CompilerError(ep, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                *type = Type::something();
                return true;
            case E_LARGE_BLUE_CIRCLE:
                *type = Type::someobject(optional);
                return true;
            case E_SPARKLES:
                throw CompilerError(ep, "The Nothingness type may not be referenced to.");
        }
    }

    EmojicodeString key = EmojicodeString(ns);
    key.append(name);
    auto it = types_.find(key);

    if (it != types_.end()) {
        auto xtype = it->second;
        if (optional) xtype.setOptional();
        *type = xtype;
        return true;
    }
    else {
        return false;
    }
}

void Package::exportType(Type t, EmojicodeString name) {
    if (finishedLoading()) {
        throw std::logic_error("The package did already finish loading. No more types can be exported.");
    }
    exportedTypes_.push_back(ExportedType(t, name));
}

void Package::registerType(Type t, EmojicodeString name, EmojicodeString ns, bool exportFromPackage) {
    EmojicodeString key = EmojicodeString(ns);
    key.append(name);
    types_.emplace(key, t);

    if (exportFromPackage) {
        exportType(t, name);
    }
}

void Package::loadInto(Package *destinationPackage, EmojicodeString ns, SourcePosition p) const {
    for (auto exported : exportedTypes_) {
        Type type = Type::nothingness();
        if (destinationPackage->fetchRawType(exported.name, ns, false, p, &type)) {
            throw CompilerError(p, "Package %s could not be loaded into namespace %s of package %s: %s collides with a type of the same name in the same namespace.", name().c_str(), ns.utf8().c_str(), destinationPackage->name().c_str(),
                                         exported.name.utf8().c_str());
        }

        destinationPackage->registerType(exported.type, exported.name, ns, false);
    }
}
