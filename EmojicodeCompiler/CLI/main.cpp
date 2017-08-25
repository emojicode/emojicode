//
//  main.c
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 27.02.15.
//  Copyright (c) 2015 Theo Weidmann. All rights reserved.
//

#include "EmojicodeCompiler.hpp"
#include "Application.hpp"
#include "CLI/PackageReporter.hpp"
#include "CLI/HRFApplicationDelegate.hpp"
#include "CLI/JSONApplicationDelegate.hpp"
#include <cstdlib>
#include <getopt.h>
#include <exception>
#include <memory>

namespace EmojicodeCompiler {
namespace CLI {

void cliWarning(const std::string &message) {
    puts(message.c_str());
}

/// The compiler CLI main function
/// @returns True iff the compilation was successful as determined by Application::compile.
bool start(int argc, char *argv[]) {
    const char *packageToReport = nullptr;
    std::string outPath;
    std::string sizeVariable;
    std::string packageDirectory = defaultPackagesDirectory;
    bool jsonOutput = false;

    if (const char *ppath = getenv("EMOJICODE_PACKAGES_PATH")) {
        packageDirectory = ppath;
    }

    signed char ch;
    while ((ch = getopt(argc, argv, "vrjR:o:S:")) != -1) {
        switch (ch) {
            case 'v':
                puts("Emojicode 0.5. Created by Theo Weidmann.");
                return false;
            case 'R':
                packageToReport = optarg;
                break;
            case 'r':
                packageToReport = "_";
                break;
            case 'o':
                outPath = optarg;
                break;
            case 'j':
                jsonOutput = true;
                break;
            case 'S':
                sizeVariable = optarg;
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
        cliWarning("No input file provided.");
        return false;
    }
    if (argc > 1) {
        cliWarning("Only the first file provided will be compiled.");
    }

    if (outPath.empty()) {
        outPath = argv[0];
        outPath[outPath.size() - 1] = 'b';
    }

    auto delegate = jsonOutput ? std::unique_ptr<ApplicationDelegate>(std::make_unique<JSONApplicationDelegate>()) :
    std::unique_ptr<ApplicationDelegate>(std::make_unique<HRFApplicationDelegate>());
    auto application = Application(argv[0], std::move(outPath), std::move(packageDirectory), delegate.get());
    bool successfullyCompiled = application.compile();

    if (packageToReport != nullptr) {
        if (auto package = application.findPackage(packageToReport)) {
            reportPackage(package);
        }
        else {
            cliWarning("Report for package %s failed as it was not loaded.");
        }
    }

    if (!sizeVariable.empty()) {
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
