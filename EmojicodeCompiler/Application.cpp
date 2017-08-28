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
    Package underscorePackage = factorUnderscorePackage<Package>();

    try {
        underscorePackage.compile();
        packagesLoadingOrder_.emplace_back(&underscorePackage);

        if (!hasStartFlagFunction()) {
            throw CompilerError(underscorePackage.position(), "No 🏁 block was found.");
        }

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
    auto package = new Package(name, path, this);
    packages_.emplace(name, package);
    packagesLoadingOrder_.emplace_back(package);
    package->compile();
    return package;
}

void Application::error(const CompilerError &ce) {
    hasError_ = true;
    delegate_->error(ce.position(), ce.message());
}

void Application::warn(const SourcePosition &p, const std::string &warning) {
    delegate_->warn(p, warning);
}

} // namespace EmojicodeCompiler
