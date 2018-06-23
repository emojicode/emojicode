//
//  Compiler.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CompilerError.hpp"
#include "Analysis/SemanticAnalyser.hpp"
#include "Compiler.hpp"
#include "Generation/CodeGenerator.hpp"
#include "Package/RecordingPackage.hpp"
#include "Parsing/AbstractParser.hpp"
#include "Parsing/CompatibilityInfoProvider.hpp"
#include "Prettyprint/PrettyPrinter.hpp"
#include <llvm/Support/FileSystem.h>

#include <utility>

namespace EmojicodeCompiler {

Compiler::Compiler(std::string mainPackage, std::string mainFile, std::string interfaceFile, std::string outPath,
                   std::string objectPath, std::string linker, std::string ar, std::vector<std::string> pkgSearchPaths,
                   std::unique_ptr<CompilerDelegate> delegate, bool pack, bool standalone)
        : pack_(pack), standalone_(standalone), mainFile_(std::move(mainFile)),
          interfaceFile_(std::move(interfaceFile)),
          outPath_(std::move(outPath)),
          mainPackageName_(std::move(mainPackage)), packageSearchPaths_(std::move(pkgSearchPaths)),
          linker_(std::move(linker)), ar_(std::move(ar)), objectPath_(std::move(objectPath)),
          delegate_(std::move(delegate)),
          mainPackage_(std::make_unique<RecordingPackage>(mainPackageName_, mainFile_, this)) {}

Compiler::~Compiler() = default;

bool Compiler::compile(bool parseOnly, bool optimize, bool printIr) {
    delegate_->begin();

    try {
        mainPackage_->parse(mainFile_);
        if (parseOnly) {
            return !hasError_;
        }

        if (!interfaceFile_.empty()) {
            PrettyPrinter(mainPackage_.get()).printInterface(interfaceFile_);
        }

        analyse();

        if (!hasError_) {
            generateCode(optimize, printIr);

            if (pack_) {
                if (standalone_) {
                    linkToExecutable();
                }
                else {
                    archive();
                }
            }

        }
    }
    catch (CompilerError &ce) {
        error(ce);
    }

    delegate_->finish();
    return !hasError_;
}

void Compiler::analyse() {
    SemanticAnalyser(mainPackage_.get(), false).analyse(standalone_);
}

void Compiler::generateCode(bool optimize, bool printIr) {
    CodeGenerator(mainPackage_.get(), optimize).generate(objectPath_, printIr);
}

void Compiler::linkToExecutable() {
    std::stringstream cmd;

    cmd << linker_ << " " << objectPath_;

    for (auto &package : packages_) {
        auto path = findBinaryPathPackage(package.second->path(), package.second->name());
        cmd << " " << path;
    }

    auto runtimeLib = findBinaryPathPackage(searchPackage("runtime", SourcePosition(0, 0, nullptr)), "runtime");
    cmd << " " << runtimeLib << " -lm -lpthread -o " << outPath_;

    system(cmd.str().c_str());
}

void Compiler::archive() {
    std::string cmd = ar_;
    cmd.append(" cr ");
    cmd.append(outPath_);
    cmd.append(" ");
    cmd.append(objectPath_);
    system(cmd.c_str());
}

std::string Compiler::searchPackage(const std::string &name, const SourcePosition &p) {
    for (auto &path : packageSearchPaths_) {
        auto full = path + "/";
        full += name;
        if (llvm::sys::fs::is_directory(full)) {
            return full;
        }
    }
    throw CompilerError(p, "Could not find package ", name, ".");
}

std::string Compiler::findBinaryPathPackage(const std::string &packagePath, const std::string &packageName) {
    return packagePath + "/lib" + packageName + ".a";
}

Package *Compiler::findPackage(const std::string &name) const {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second.get() : nullptr;
}

Package *Compiler::loadPackage(const std::string &name, const SourcePosition &p, Package *requestor) {
    if (auto package = findPackage(name)) {
        if (!package->finishedLoading()) {
            throw CompilerError(p, "Circular dependency detected: ", requestor->name(), " and ", name,
                                " depend on each other.");
        }
        return package;
    }

    auto package = std::make_unique<Package>(name, searchPackage(name, p), this);
    auto rawPtr = package.get();
    packages_.emplace(name, std::move(package));
    rawPtr->parse();
    SemanticAnalyser(rawPtr, true).analyse(false);
    return rawPtr;
}

void Compiler::error(const CompilerError &ce) {
    hasError_ = true;
    delegate_->error(this, ce.message(), ce.position());
}

void Compiler::warn(const SourcePosition &p, const std::string &warning) {
    delegate_->warn(this, warning, p);
}

Class *getStandardClass(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), &type);
    if (type.type() != TypeType::Class) {
        throw CompilerError(errorPosition, "s package class ", utf8(name), " is missing.");
    }
    return type.klass();
}

Protocol *getStandardProtocol(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), &type);
    if (type.unboxedType() != TypeType::Protocol) {
        throw CompilerError(errorPosition, "s package protocol ", utf8(name), " is missing.");
    }
    return type.protocol();
}

ValueType *getStandardValueType(const std::u32string &name, Package *_, const SourcePosition &errorPosition) {
    Type type = Type::noReturn();
    _->lookupRawType(TypeIdentifier(name, kDefaultNamespace, errorPosition), &type);
    if (type.type() != TypeType::ValueType) {
        throw CompilerError(errorPosition, "s package value type ", utf8(name), " is missing.");
    }
    return type.valueType();
}

void Compiler::assignSTypes(Package *s, const SourcePosition &errorPosition) {
    // Order of the following calls is important as they will cause Box IDs to be assigned
    sBoolean = getStandardValueType(U"👌", s, errorPosition);
    sInteger = getStandardValueType(U"🔢", s, errorPosition);
    sReal = getStandardValueType(std::u32string(1, E_HUNDRED_POINTS_SYMBOL), s, errorPosition);
    sSymbol = getStandardValueType(U"🔣", s, errorPosition);
    sMemory = getStandardValueType(U"🧠", s, errorPosition);
    sByte = getStandardValueType(U"💧", s, errorPosition);

    sString = getStandardClass(U"🔡", s, errorPosition);
    sList = getStandardClass(U"🍨", s, errorPosition);
    sDictionary = getStandardClass(U"🍯", s, errorPosition);

    sEnumerable = getStandardProtocol(
            std::u32string(1, E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY), s,
            errorPosition);
}

void Compiler::loadMigrationFile(const std::string &file) {
    mainPackage_->setCompatiblityInfoProvider(compInfoProvider_.get());
    compInfoProvider_ = std::make_unique<CompatibilityInfoProvider>(file);
}

} // namespace EmojicodeCompiler
