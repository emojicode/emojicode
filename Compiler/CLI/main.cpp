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
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

/// The compiler CLI main function
/// @returns True if the requested operation was successful.
bool start(const Options &options) {
    Compiler application(options.mainPackageName(), options.mainFile(), options.interfaceFile(), options.outPath(),
                         options.linker(), options.packageSearchPaths(), options.compilerDelegate(),
                         options.linkToExec());

    if (!options.migrationFile().empty()) {
        application.loadMigrationFile(options.migrationFile());
    }

    bool success = application.compile(options.prettyprint(), options.optimize());

    if (options.prettyprint()) {
        Prettyprinter(application.mainPackage()).print();
    }

    if (options.shouldReport()) {
        reportPackage(application.mainPackage());
    }
    return success;
}

}  // namespace CLI

}  // namespace EmojicodeCompiler

int main(int argc, char *argv[]) {
    try {
        return EmojicodeCompiler::CLI::start(EmojicodeCompiler::CLI::Options(argc, argv)) ? 0 : 1;
    }
    catch (EmojicodeCompiler::CLI::CompilationCancellation &e) { return 0; }
    catch (std::exception &ex) {
        std::cout << "💣 The compiler crashed due to an internal problem: " << ex.what() << std::endl;
        std::cout << "Please report this message and the code that you were trying to compile as an issue on GitHub.";
        std::cout << std::endl;
        return 70;
    }
    catch (...) {
        std::cout << "💣 The compiler crashed due to an unidentifiable internal problem." << std::endl;
        std::cout << "Please report this message and the code that you were trying to compile as an issue on GitHub.";
        std::cout << std::endl;
        return 70;
    }
}
