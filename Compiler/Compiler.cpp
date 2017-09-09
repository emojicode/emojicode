//
//  Compiler.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Compiler.hpp"
#include "Generation/CodeGenerator.hpp"

namespace EmojicodeCompiler {

Compiler::Compiler(std::string mainPackage, std::string mainFile, std::string outPath, std::string pkgDir,
                         std::unique_ptr<CompilerDelegate> delegate, bool standalone)
: standalone_(standalone), mainFile_(std::move(mainFile)), outPath_(std::move(outPath)),
    mainPackageName_(std::move(mainPackage)), packageDirectory_(std::move(pkgDir)), delegate_(std::move(delegate)){}

bool Compiler::compile(bool parseOnly) {
    delegate_->begin();

    factorMainPackage<Package>();

    try {
        mainPackage_->parse();
        if (parseOnly) {
            return !hasError_;
        }
        analyse();
    }
    catch (CompilerError &ce) {
        error(ce);
    }

    if (!hasError_) {
        generateCode();
    }

    delegate_->finish();
    return !hasError_;
}

void Compiler::analyse() {
    mainPackage_->analyse();

    if (standalone_ && !mainPackage_->hasStartFlagFunction()) {
        throw CompilerError(mainPackage_->position(), "No 🏁 block was found.");
    }
}

void Compiler::generateCode() {
    CodeGenerator(mainPackage_.get()).generate(outPath_);
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

    auto path = packageDirectory_ + "/" + name + "/header.emojic";
    auto package = std::make_unique<Package>(name, path, this);
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

} // namespace EmojicodeCompiler
