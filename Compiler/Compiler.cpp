//
//  Compiler.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CompilerError.hpp"
#include "Compiler.hpp"
#include "Generation/CodeGenerator.hpp"
#include "Prettyprint/Prettyprinter.hpp"
#include "Parsing/AbstractParser.hpp"
#include <llvm/Support/FileSystem.h>

#include <utility>

namespace EmojicodeCompiler {

Compiler::Compiler(std::string mainPackage, std::string mainFile, std::string interfaceFile, std::string outPath,
                   std::string linker, std::vector<std::string> pkgSearchPaths,
                   std::unique_ptr<CompilerDelegate> delegate, bool linkToExec)
: linkToExec_(linkToExec), mainFile_(std::move(mainFile)), interfaceFile_(std::move(interfaceFile)), outPath_(std::move(outPath)),
  mainPackageName_(std::move(mainPackage)), packageSearchPaths_(std::move(pkgSearchPaths)), linker_(std::move(linker)),
  delegate_(std::move(delegate)) {}

bool Compiler::compile(bool parseOnly) {
    delegate_->begin();

    factorMainPackage<RecordingPackage>();

    try {
        mainPackage_->parse(mainFile_);
        if (parseOnly) {
            return !hasError_;
        }

        if (!interfaceFile_.empty()) {
            Prettyprinter(dynamic_cast<RecordingPackage *>(mainPackage_.get())).printInterface(interfaceFile_);
        }

        analyse();

        if (!hasError_) {
            generateCode();

            if (linkToExec_) {
                linkToExecutable();
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
    mainPackage_->analyse();

    if (linkToExec_ && !mainPackage_->hasStartFlagFunction()) {
        throw CompilerError(mainPackage_->position(), "No 🏁 block was found.");
    }
}

void Compiler::generateCode() {
    CodeGenerator(mainPackage_.get()).generate(objectFileName());
}

std::string Compiler::objectFileName() const {
    return linkToExec_ ? outPath_ + ".o" : outPath_;
}

void Compiler::linkToExecutable() {
    std::stringstream cmd;

    cmd << linker_ << " " << objectFileName();

    for (auto &package : packages_) {
        auto path = findBinaryPathPackage(package.second->path(), package.second->name());
        if (llvm::sys::fs::exists(path)) {
            cmd << " " << path;
        }
    }

    auto runtimeLib = findBinaryPathPackage(searchPackage("runtime", SourcePosition(0, 0, mainFile_)), "runtime");
    cmd << " " << runtimeLib << " -lm -o " << outPath_;

    system(cmd.str().c_str());
}

std::string Compiler::searchPackage(const std::string &name, const SourcePosition &p) {
    for (auto &path : packageSearchPaths_) {
        auto full = path + "/" + name;
        if (llvm::sys::fs::is_directory(full)) {
            return full;
        }
    }
    throw CompilerError(p, "Could not find package ", name, ".");
}

std::string Compiler::findBinaryPathPackage(const std::string &packagePath, const std::string &packageName) {
    return packagePath + "/lib" + packageName + ".a";
}

Package* Compiler::findPackage(const std::string &name) const {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second.get() : nullptr;
}

Package* Compiler::loadPackage(const std::string &name, const SourcePosition &p, Package *requestor) {
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
    rawPtr->analyse();
    return rawPtr;
}

void Compiler::error(const CompilerError &ce) {
    hasError_ = true;
    delegate_->error(ce.position(), ce.message());
}

void Compiler::warn(const SourcePosition &p, const std::string &warning) {
    delegate_->warn(p, warning);
}

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

void Compiler::assignSTypes(Package *s, const SourcePosition &errorPosition) {
    // Order of the following calls is important as they will cause Box IDs to be assigned
    sBoolean = getStandardValueType(std::u32string(1, E_OK_HAND_SIGN), s, errorPosition);
    sInteger = getStandardValueType(std::u32string(1, E_INPUT_SYMBOL_FOR_NUMBERS), s, errorPosition);
    sReal = getStandardValueType(std::u32string(1, E_HUNDRED_POINTS_SYMBOL), s, errorPosition);
    sSymbol = getStandardValueType(std::u32string(1, E_INPUT_SYMBOL_FOR_SYMBOLS), s, errorPosition);
    sMemory = getStandardValueType(std::u32string(1, E_BRAIN), s, errorPosition);

    sString = getStandardClass(std::u32string(1, 0x1F521), s, errorPosition);
    sList = getStandardClass(std::u32string(1, 0x1F368), s, errorPosition);
    sData = getStandardClass(std::u32string(1, 0x1F4C7), s, errorPosition);
    sDictionary = getStandardClass(std::u32string(1, 0x1F36F), s, errorPosition);

    sEnumerator = getStandardProtocol(std::u32string(1, 0x1F361), s, errorPosition);
    sEnumeratable = getStandardProtocol(std::u32string(1, E_CLOCKWISE_RIGHTWARDS_AND_LEFTWARDS_OPEN_CIRCLE_ARROWS_WITH_CIRCLED_ONE_OVERLAY), s, errorPosition);
}

} // namespace EmojicodeCompiler
