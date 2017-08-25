//
//  Application.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Application.hpp"
#include "Parsing/Package.hpp"
#include "Generation/Writer.hpp"
#include "Generation/CodeGenerator.hpp"

namespace EmojicodeCompiler {

bool Application::compile() {
    Package underscorePackage = Package("_", mainFile_, this);
    underscorePackage.setPackageVersion(PackageVersion(1, 0));
    underscorePackage.setRequiresBinary(false);

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
            return true;
        }
    }
    catch (CompilerError &ce) {
        error(ce);
    }
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
//    if (outputJSON) {
//        fprintf(stderr, "%s{\"type\": \"error\", \"line\": %zu, \"character\": %zu, \"file\":",
//                printedErrorOrWarning ? ",": "", ce.position().line, ce.position().character);
//        printJSONStringToFile(ce.position().file.c_str(), stderr);
//        fprintf(stderr, ", \"message\":");
//        printJSONStringToFile(ce.message().c_str(), stderr);
//        fprintf(stderr, "}\n");
//    }
//    else {
        fprintf(stderr, "🚨 line %zu column %zu %s: %s\n", ce.position().line, ce.position().character,
                ce.position().file.c_str(), ce.message().c_str());
//    }
//    printedErrorOrWarning = true;
}

void Application::warn(const SourcePosition &p, const std::string &warning) {
//    if (outputJSON) {
//        fprintf(stderr, "%s{\"type\": \"warning\", \"line\": %zu, \"character\": %zu, \"file\":",
//                printedErrorOrWarning ? ",": "", p.line, p.character);
//        printJSONStringToFile(p.file.c_str(), stderr);
//        fprintf(stderr, ", \"message\":");
//        printJSONStringToFile(warning.c_str(), stderr);
//        fprintf(stderr, "}\n");
//    }
//    else {
        fprintf(stderr, "⚠️ line %zu col %zu %s: %s\n", p.line, p.character, p.file.c_str(), warning.c_str());
//    }
//    printedErrorOrWarning = true;
}

}
