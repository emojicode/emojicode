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
#include "Parsing/DocumentParser.hpp"
#include "Types/Class.hpp"
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

Package::Package(std::string name, std::string path, Compiler *app, bool imported)
    : name_(std::move(name)), path_(std::move(path)), imported_(imported), compiler_(app) {}
Package::~Package() = default;

void Package::importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) {
    auto import = compiler_->loadPackage(name, p, this);
    for (auto exported : import->exportedTypes_) {
        Type type = Type::noReturn();
        if (lookupRawType(TypeIdentifier(exported.name, ns, p), &type)) {
            throw CompilerError(p, "Package ", name , " could not be loaded into namespace ", utf8(ns),
                                " of package ", name_, ": ", utf8(exported.name),
                                " collides with a type of the same name in the same namespace.");
        }

        offerType(exported.type, exported.name, ns, false, p);
    }
}

TokenStream Package::lexFile(const std::string &path) {
    if (!endsWith(path, ".emojic") && !endsWith(path, ".🍇") && !endsWith(path, "🏛") && !endsWith(path, ".emojii")) {
        throw CompilerError(SourcePosition(), "Emojicode files must be suffixed with .emojic or .🍇: ", path);
    }

    // Exclude "extension only" filename from compilation
    std::size_t extensionIndex = path.find_last_of("."); 
    std::string stem = path.substr(0, extensionIndex);
    if (endsWith(stem, "/")) {
        throw CompilerError(SourcePosition(), "Emojicode files must have a filename: ", path);
    }

    return TokenStream(Lexer(compiler()->sourceManager().read(path)));
}

void Package::includeDocument(const std::string &path, const std::string &relativePath) {
    DocumentParser(this, lexFile(path), endsWith(path, "🏛") || endsWith(path, "emojii")).parse();
}

void Package::parse(const std::string &mainFilePath) {
    if (name_ != "s") {
        importPackage("s", kDefaultNamespace, SourcePosition());
    }
    
    includeDocument(mainFilePath, "");

    if (name() == "s") {
        compiler()->assignSTypes(this);
    }
    finishedLoading_ = true;
}

Type Package::getRawType(const TypeIdentifier &typeId) const {
    auto type = Type::noReturn();
    if (!lookupRawType(typeId, &type)) {
        throw CompilerError(typeId.position, "Could not find a type named ", utf8(typeId.name), " in namespace ",
                            utf8(typeId.getNamespace()), ".");
    }
    return type;
}

bool Package::lookupRawType(const TypeIdentifier &typeId, Type *type) const {
    std::u32string key = typeId.getNamespace() + typeId.name;
    auto it = types_.find(key);

    if (it != types_.end()) {
        *type = it->second;
        return true;
    }
    return false;
}

void Package::exportType(Type t, std::u32string name, const SourcePosition &p) {
    assert(!finishedLoading());
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

void Package::recreateClassTypes() {
    for (auto &pair : types_) {
        if (pair.second.type() == TypeType::Class) {
            pair.second = Type(pair.second.klass());
        }
        else if (pair.second.type() == TypeType::ValueType) {
            pair.second = Type(pair.second.valueType());
        }
    }
}

}  // namespace EmojicodeCompiler
