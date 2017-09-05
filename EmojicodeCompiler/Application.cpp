//
//  Application.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Application.hpp"
#include "Generation/CodeGenerator.hpp"

namespace EmojicodeCompiler {

Application::Application(std::string mainFile, std::string outPath, std::string pkgDir,
                         std::unique_ptr<ApplicationDelegate> delegate)
: mainFile_(std::move(mainFile)), outPath_(std::move(outPath)), packageDirectory_(std::move(pkgDir)),
    delegate_(std::move(delegate)) {}

bool Application::compile(bool parseOnly) {
    delegate_->begin();

    try {
        factorUnderscorePackage<Package>();
        underscorePackage_->parse();
        if (parseOnly) {
            return !hasError_;
        }
        analyse(underscorePackage_.get());
        packagesLoadingOrder_.emplace_back(std::move(underscorePackage_));
        underscorePackage_ = nullptr;
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

void Application::analyse(Package *underscorePackage) {
    underscorePackage->analyse();

    if (!hasStartFlagFunction()) {
        throw CompilerError(underscorePackage->position(), "No 🏁 block was found.");
    }
}

void Application::generateCode() {
    CodeGenerator(underscorePackage()).generate(outPath_);
}

Package* Application::findPackage(const std::string &name) const {
    auto it = packages_.find(name);
    return it != packages_.end() ? it->second : nullptr;
}

Package* Application::loadPackage(const std::string &name, const SourcePosition &p, Package *requestor) {
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
    packages_.emplace(name, rawPtr);
    packagesLoadingOrder_.emplace_back(std::move(package));
    rawPtr->parse();
    rawPtr->analyse();
    return rawPtr;
}

void Application::error(const CompilerError &ce) {
    hasError_ = true;
    delegate_->error(ce.position(), ce.message());
}

void Application::warn(const SourcePosition &p, const std::string &warning) {
    delegate_->warn(p, warning);
}

} // namespace EmojicodeCompiler
