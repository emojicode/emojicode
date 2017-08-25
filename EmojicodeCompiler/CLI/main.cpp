//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "../Application.hpp"
#include "PackageReporter.hpp"
#include "Options.hpp"
#include <exception>

namespace EmojicodeCompiler {

namespace CLI {

/// The compiler CLI main function
/// @returns True iff the compilation was successful as determined by Application::compile.
bool start(int argc, char *argv[]) {
    Options options(argc, argv);

    if (!options.beginCompilation()) {
        return false;
    }

    auto application = Application(options.mainFile(), options.outPath(), options.packageDirectory(),
                                   options.applicationDelegate());
    bool successfullyCompiled = application.compile();

    if (!options.packageToReport().empty()) {
        if (auto package = application.findPackage(options.packageToReport())) {
            reportPackage(package);
        }
        else {
            options.printCliMessage("Report failed as the request package was not loaded.");
        }
    }

    if (!options.sizeVariable().empty()) {
        //            InformationDesk(&pkg).sizeOfVariable(sizeVariable);
    }

    return successfullyCompiled;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler

int main(int argc, char *argv[]) {
    try {
        return EmojicodeCompiler::CLI::start(argc, argv) ? 0 : 1;
    }
    catch (std::exception &ex) {
        printf("💣 The compiler crashed due to an internal problem: %s\nPlease report this message and the code that "
               "you were trying to compile as an issue on GitHub.", ex.what());
        return 1;
    }
    catch (...) {
        printf("💣 The compiler crashed due to an unidentifiable internal problem.\nPlease report this message and the "
               "code that you were trying to compile as an issue on GitHub.");
        return 1;
    }
    return 0;
}
