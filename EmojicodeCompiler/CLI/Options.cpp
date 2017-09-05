//
//  Options.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Options.hpp"
#include "HRFApplicationDelegate.hpp"
#include "JSONApplicationDelegate.hpp"
#include <cstdlib>
#include <getopt.h>
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

Options::Options(int argc, char *argv[]) {
    readEnvironment();

    signed char ch;
    while ((ch = getopt(argc, argv, "vrjR:o:S:f")) != -1) {
        switch (ch) {
            case 'v':
                puts("Emojicode 0.5. Created by Theo Weidmann.");
                beginCompilation_ = false;
                return;
            case 'R':
                packageToReport_ = optarg;
                break;
            case 'r':
                packageToReport_ = "_";
                break;
            case 'o':
                outPath_ = optarg;
                break;
            case 'j':
                jsonOutput_ = true;
                break;
            case 'S':
                sizeVariable_ = optarg;
                break;
            case 'f':
                format_ = true;
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    parsePositionalArguments(argc, argv);
}

void Options::readEnvironment() {
    if (const char *ppath = getenv("EMOJICODE_PACKAGES_PATH")) {
        packageDirectory_ = ppath;
    }
}

void Options::printCliMessage(const std::string &message) {
    std::cout << "👉  " << message << std::endl;
}

void Options::parsePositionalArguments(int positionalArguments, char *argv[]) {
    if (positionalArguments == 0) {
        printCliMessage("No input file provided.");
        beginCompilation_ = false;
        return;
    }
    if (positionalArguments > 1) {
        printCliMessage("Only the first document provided will be compiled.");
    }

    mainFile_ = argv[0];

    examineMainFile();
}

void Options::examineMainFile() {
    if (endsWith(mainFile_, ".emojimig")) {
        migrationFile_ = mainFile_;
        format_ = true;
        mainFile_.replace(mainFile_.size() - 8, 8, "emojic");
    }

    if (outPath_.empty()) {
        outPath_ = mainFile_;
        outPath_.replace(mainFile_.size() - 6, 6, "o");
    }
}

std::unique_ptr<ApplicationDelegate> Options::applicationDelegate() {
    if (jsonOutput_) {
        return std::make_unique<JSONApplicationDelegate>();
    }
    return std::make_unique<HRFApplicationDelegate>();
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
