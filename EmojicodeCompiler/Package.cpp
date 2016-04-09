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
#include "FileParser.hpp"
#include "utf8.h"

std::list<Package *> Package::packagesLoadingOrder_;
std::map<std::string, Package *> Package::packages_;

Package* Package::loadPackage(const char *name, EmojicodeChar ns, const Token *errorToken) {
    Package *package = findPackage(name);
    
    if (package) {
        if (!package->finishedLoading()) {
            compilerError(errorToken, "Circular dependency detected: %s tried to load a package which intiatiated %s’s own loading.", name, name);
        }
    }
    else {
        char *path;
        asprintf(&path, packageDirectory "%s/header.emojic", name);
        
        package = new Package(name);
        
        if (strcmp("s", name) != 0) {
            package->loadPackage("s", globalNamespace, errorToken);
        }
        package->parse(path, errorToken);
    }
    
    package->loadInto(this, ns, errorToken);
    return package;
}

void Package::parse(const char *path, const Token *errorToken) {
    packages_.emplace(name(), this);
    
    parseFile(path, this);
    
    if (version().major == 0 && version().minor == 0) {
        compilerError(errorToken, "Package %s does not provide a valid version.", name());
    }
    
    packagesLoadingOrder_.push_back(this);
    
    finishedLoading_ = true;
}

Package* Package::findPackage(const char *name) {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second : nullptr;
}

bool Package::fetchRawType(EmojicodeChar name, EmojicodeChar ns, bool optional, const Token *token, Type *type) {
    if (ns == globalNamespace) {
        switch (name) {
            case E_OK_HAND_SIGN:
                *type = Type(TT_BOOLEAN, optional);
                return true;
            case E_INPUT_SYMBOL_FOR_SYMBOLS:
                *type = Type(TT_SYMBOL, optional);
                return true;
            case E_STEAM_LOCOMOTIVE:
                *type = Type(TT_INTEGER, optional);
                return true;
            case E_ROCKET:
                *type = Type(TT_DOUBLE, optional);
                return true;
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    compilerWarning(token, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                *type = Type(TT_SOMETHING, false);
                return true;
            case E_LARGE_BLUE_CIRCLE:
                *type = Type(TT_SOMEOBJECT, optional);
                return true;
            case E_SPARKLES:
                compilerError(token, "The Nothingness type may not be referenced to.");
        }
    }
    
    std::array<EmojicodeChar, 2> key = {ns, name};
    auto it = types_.find(key);
    
    if (it != types_.end()) {
        auto xtype = it->second;
        xtype.optional = optional;
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

void Package::loadInto(Package *destinationPackage, EmojicodeChar ns, const Token *errorToken) const {
    for (auto exported : exportedTypes_) {
        Type type = typeNothingness;
        if (destinationPackage->fetchRawType(exported.name, ns, false, nullptr, &type)) {
            ecCharToCharStack(ns, nss);
            ecCharToCharStack(exported.name, tname);
            compilerError(errorToken, "Package %s could not be loaded into namespace %s of package %s: %s collides with a type of the same name in the same namespace.", name(), nss, destinationPackage->name(), tname);
        }
        
        destinationPackage->registerType(exported.type, exported.name, ns, false);
    }
}