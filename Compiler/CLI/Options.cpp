//
//  Options.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Options.hpp"
#include "HRFCompilerDelegate.hpp"
#include "JSONCompilerDelegate.hpp"
#include <cstdlib>
#include <getopt.h>
#include <llvm/Support/Path.h>
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

Options::Options(int argc, char *argv[]) {
    readEnvironment();

    signed char ch;
    while ((ch = getopt(argc, argv, "vrjR:o:p:f")) != -1) {
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
            case 'p':
                mainPackageName_ = optarg;
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

    if (mainPackageName_.empty()) {
        mainPackageName_ = "_";
        standalone_ = true;
    }

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

    std::string parentPath = llvm::sys::path::parent_path(mainFile_);
    interfaceFile_ = parentPath + "/" + "interface.emojii";
}

std::unique_ptr<CompilerDelegate> Options::applicationDelegate() {
    if (jsonOutput_) {
        return std::make_unique<JSONCompilerDelegate>();
    }
    return std::make_unique<HRFCompilerDelegate>();
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
