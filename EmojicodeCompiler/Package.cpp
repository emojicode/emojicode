//
//  Package.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Package.hpp"
#include "FileParser.hpp"
#include "utf8.h"

std::list<Package *> Package::packagesLoadingOrder_;
std::map<std::string, Package *> Package::packages_;

void Package::loadPackage(const char *name, EmojicodeChar ns, const Token *errorToken) {
    Package *package;
    
    auto it = packages_.find(name);
    if (it != packages_.end()) {
        package = it->second;
        
        if (!package->finishedLoading()) {
            compilerError(errorToken, "Circular depdency detected: %s tried to load a package which intiatiated %s’s own loading.", name);
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
}

void Package::parse(const char *path, const Token *errorToken) {
    packages_.emplace(name(), this);
    
    parseFile(path, this, true);
    
    if(version().major == 0 && version().minor == 0){
        compilerError(errorToken, "Package %s does not provide a valid version.", name());
    }
    
    packagesLoadingOrder_.push_back(this);
    
    finishedLoading_ = true;
}

Type Package::fetchRawType(EmojicodeChar name, EmojicodeChar ns, bool optional, const Token *token, bool *existent) {
    *existent = true;
    if(ns == globalNamespace){
        switch (name) {
            case E_OK_HAND_SIGN:
                return Type(TT_BOOLEAN, optional);
            case E_INPUT_SYMBOL_FOR_SYMBOLS:
                return Type(TT_SYMBOL, optional);
            case E_STEAM_LOCOMOTIVE:
                return Type(TT_INTEGER, optional);
            case E_ROCKET:
                return Type(TT_DOUBLE, optional);
            case E_MEDIUM_WHITE_CIRCLE:
                if (optional) {
                    compilerWarning(token, "🍬⚪️ is identical to ⚪️. Do not specify 🍬.");
                }
                return Type(TT_SOMETHING, false);
            case E_LARGE_BLUE_CIRCLE:
                return Type(TT_SOMEOBJECT, optional);
            case E_SPARKLES:
                compilerError(token, "The Nothingness type may not be referenced to.");
        }
    }
    
    std::array<EmojicodeChar, 2> key = {ns, name};
    auto it = types_.find(key);
    
    if (it != types_.end()) {
        auto type = it->second;
        type.optional = optional;
        return type;
    }
    else {
        *existent = false;
        return typeNothingness;
    }
}

void Package::exportType(Type t, EmojicodeChar name) {
    if (finishedLoading()) {
        throw std::logic_error("The package did already finish loading. No more types can be exported.");
    }
    exportedTypes_.push_back(ExportedType(t, name));
}

void Package::registerType(Type t, EmojicodeChar name, EmojicodeChar ns) {
    std::array<EmojicodeChar, 2> key = {ns, name};
    types_.emplace(key, t);
}

void Package::loadInto(Package *destinationPackage, EmojicodeChar ns, const Token *errorToken) const {
    for (auto exported : exportedTypes_) {
        bool existent;
        destinationPackage->fetchRawType(exported.name, ns, false, nullptr, &existent);
        if (existent) {
            ecCharToCharStack(ns, nss);
            ecCharToCharStack(exported.name, tname);
            compilerError(errorToken, "Package %s could not be loaded into namespace %s of package %s: %s collides with a type of the same name in the same namespace.", name(), nss, destinationPackage->name(), tname);
        }
        
        destinationPackage->registerType(exported.type, exported.name, ns);
    }
}