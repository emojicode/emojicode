//
//  Application.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Application.hpp"
#include "Generation/CodeGenerator.hpp"
#include "Generation/Writer.hpp"

namespace EmojicodeCompiler {

bool Application::compile() {
    delegate_->begin();
    auto underscorePackage = factorUnderscorePackage<Package>();

    try {
        underscorePackage->compile();

        if (!hasStartFlagFunction()) {
            throw CompilerError(underscorePackage->position(), "No 🏁 block was found.");
        }

        packagesLoadingOrder_.emplace_back(std::move(underscorePackage));

        if (!hasError_) {
            Writer writer = Writer(outPath_);
            generateCode(&writer, this);
            writer.finish();
            delegate_->finish();
            return true;
        }
    }
    catch (CompilerError &ce) {
        error(ce);
    }
    delegate_->finish();
    return false;
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
    packages_.emplace(name, package.get());
    packagesLoadingOrder_.emplace_back(std::move(package));
    package->compile();
    return package.get();
}

void Application::error(const CompilerError &ce) {
    hasError_ = true;
    delegate_->error(ce.position(), ce.message());
}

void Application::warn(const SourcePosition &p, const std::string &warning) {
    delegate_->warn(p, warning);
}

} // namespace EmojicodeCompiler
