//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "Compiler.hpp"
#include "Options.hpp"
#include "Package/RecordingPackage.hpp"
#include "PackageReporter.hpp"
#include "Prettyprint/Prettyprinter.hpp"
#include <exception>

namespace EmojicodeCompiler {

namespace CLI {

/// The compiler CLI main function
/// @returns True if the requested operation was successful.
bool start(Options options) {
    if (!options.beginCompilation()) {
        return true;
    }

    Compiler application(options.mainPackageName(), options.mainFile(), options.interfaceFile(), options.outPath(),
                         options.linker(), options.packageSearchPaths(), options.compilerDelegate(),
                         options.linkToExec());

    if (!options.migrationFile().empty()) {
        application.loadMigrationFile(options.migrationFile());
    }
    if (options.prettyprint()) {
        application.factorMainPackage<RecordingPackage>();
    }

    bool success = application.compile(options.prettyprint());

    if (options.prettyprint()) {
        auto recordingPackage = dynamic_cast<RecordingPackage *>(application.mainPackage());
        Prettyprinter(recordingPackage).print();
    }

    if (!options.packageToReport().empty()) {
        if (auto package = application.findPackage(options.packageToReport())) {
            reportPackage(package);
        }
        else {
            options.printCliMessage("Report failed as the request package was not loaded.");
        }
    }

    return success;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler

int main(int argc, char *argv[]) {
    try {
        return EmojicodeCompiler::CLI::start(EmojicodeCompiler::CLI::Options(argc, argv)) ? 0 : 1;
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
