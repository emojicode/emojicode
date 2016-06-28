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
#include "CompilerErrorException.hpp"
#include "utf8.h"

std::list<Package *> Package::packagesLoadingOrder_;
std::map<std::string, Package *> Package::packages_;

Package* Package::loadPackage(const char *name, EmojicodeChar ns, SourcePosition errorPosition) {
    Package *package = findPackage(name);
    
    if (package) {
        if (!package->finishedLoading()) {
            throw CompilerErrorException(errorPosition,
                                         "Circular dependency detected: %s tried to load a package which intiatiated %s’s own loading.",
                                         name, name);
        }
    }
    else {
        char *path;
        asprintf(&path, "%s/%s/header.emojic", packageDirectory, name);
        
        package = new Package(name, errorPosition);
        
        if (strcmp("s", name) != 0) {
            package->loadPackage("s", globalNamespace, errorPosition);
        }
        package->parse(path);
    }
    
    package->loadInto(this, ns, errorPosition);
    return package;
}

void Package::parse(const char *path) {
    packages_.emplace(name(), this);
    
    PackageParser(this, lex(path)).parse();
    
    if (!validVersion()) {
        throw CompilerErrorException(SourcePosition(0, 0, path), "Package %s does not provide a valid version.", name());
    }
    
    packagesLoadingOrder_.push_back(this);
    
    finishedLoading_ = true;
}

Package* Package::findPackage(const char *name) {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second : nullptr;
}

bool Package::fetchRawType(EmojicodeChar name, EmojicodeChar ns, bool optional, SourcePosition ep, Type *type) {
    if (ns == globalNamespace) {
        switch (name) {
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    compilerWarning(ep, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                *type = Type(TypeContent::Something, false);
                return true;
            case E_LARGE_BLUE_CIRCLE:
                *type = Type(TypeContent::Someobject, optional);
                return true;
            case E_SPARKLES:
                throw CompilerErrorException(ep, "The Nothingness type may not be referenced to.");
        }
    }
    
    std::array<EmojicodeChar, 2> key = {ns, name};
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

void Package::exportType(Type t, EmojicodeChar name) {
    if (finishedLoading()) {
        throw std::logic_error("The package did already finish loading. No more types can be exported.");
    }
    exportedTypes_.push_back(ExportedType(t, name));
}

void Package::registerType(Type t, EmojicodeChar name, EmojicodeChar ns, bool exportFromPackage) {
    std::array<EmojicodeChar, 2> key = {ns, name};
    types_.emplace(key, t);
    
    if (exportFromPackage) {
        exportType(t, name);
    }
}

void Package::loadInto(Package *destinationPackage, EmojicodeChar ns, const Token &errorToken) const {
    for (auto exported : exportedTypes_) {
        Type type = typeNothingness;
        if (destinationPackage->fetchRawType(exported.name, ns, false, errorToken, &type)) {
            ecCharToCharStack(ns, nss);
            ecCharToCharStack(exported.name, tname);
            throw CompilerErrorException(errorToken, "Package %s could not be loaded into namespace %s of package %s: %s collides with a type of the same name in the same namespace.", name(), nss, destinationPackage->name(), tname);
        }
        
        destinationPackage->registerType(exported.type, exported.name, ns, false);
    }
}