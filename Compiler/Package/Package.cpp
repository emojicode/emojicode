//
//  Package.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 01/04/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "Scoping/SemanticScoper.hpp"
#include "Compiler.hpp"
#include "CompilerError.hpp"
#include "Lex/Lexer.hpp"
#include "Lex/SourceManager.hpp"
#include "Package.hpp"
#include "Parsing/CompatibilityInfoProvider.hpp"
#include "Parsing/DocumentParser.hpp"
#include "Types/Class.hpp"
#include "Types/Extension.hpp"
#include "Types/Protocol.hpp"
#include "Types/ValueType.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>
#include <string>

namespace EmojicodeCompiler {

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

Package::Package(std::string name, std::string path, Compiler *app)
    : name_(std::move(name)), path_(std::move(path)), compiler_(app) {}
Package::~Package() = default;

void Package::importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) {
    auto import = compiler_->loadPackage(name, p, this);

    importedPackages_.emplace(import);

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

TokenStream Package::lexFile(const std::string &path) {
    if (!endsWith(path, ".emojic") && !endsWith(path, ".emojii")) {
        throw CompilerError(SourcePosition(0, 0, path), "Emojicode files must be suffixed with .emojic: ", path);
    }
    return TokenStream(Lexer(compiler()->sourceManager().read(path), path));
}

void Package::includeDocument(const std::string &path, const std::string &relativePath) {
    DocumentParser(this, lexFile(path), endsWith(path, "emojii")).parse();
}

void Package::parse() {
    parse(path_ + "/interface.emojii");
}

void Package::parse(const std::string &mainFilePath) {
    if (name_ != "s") {
        importPackage("s", kDefaultNamespace, position());
    }
    
    includeDocument(mainFilePath, "");

    if (!validVersion()) {
        compiler_->warn(position(), "Package ", name(), " does not provide a valid version.");
    }
    if (name() == "s") {
        compiler()->assignSTypes(this, position());
    }
    finishedLoading_ = true;
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
    std::u32string key = typeId.ns + typeId.name;
    auto it = types_.find(key);

    if (it != types_.end()) {
        auto xtype = it->second;
        if (optional) {
            xtype = Type(MakeOptional, xtype);
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
