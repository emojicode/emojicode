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
#include "Utils/StringUtils.hpp"
#include "Utils/args.hxx"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

#ifndef defaultPackagesDirectory
#define defaultPackagesDirectory "/usr/local/EmojicodePackages"
#endif

Options::Options(int argc, char *argv[]) {
    readEnvironment();

    args::ArgumentParser parser("Emojicode Compiler 0.6.");
    args::Positional<std::string> file(parser, "file", "The main file of the package to be compiled", std::string(),
                                       args::Options::Required);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> package(parser, "package", "The name of the package", {'p'});
    args::ValueFlag<std::string> out(parser, "out", "Output path for binary or assembly", {'o'});
    args::ValueFlag<std::string> interfaceOut(parser, "interface", "Output interface to given path", {'i'});
    args::Flag report(parser, "report", "Generate a JSON report about the package", {'r'});
    args::Flag object(parser, "object", "Produce object file, do not link", {'c'});
    args::Flag json(parser, "json", "Show compiler messages as JSON", {"json"});
    args::Flag format(parser, "format", "Format source code", {"format"});
    args::Flag color(parser, "color", "Show compiler messages in color.", {"color"});

    try {
        parser.ParseCLI(argc, argv);

        report_ = report.Get();
        mainFile_ = file.Get();
        jsonOutput_ = json.Get();
        format_ = format.Get();
        forceColor_ = color.Get();

        if (package) {
            mainPackageName_ = package.Get();
            linkToExec_ = false;
        }
        if (object) {
            linkToExec_ = false;
        }
        if (out) {
            outPath_ = out.Get();
        }
        if (interfaceOut) {
            interfaceFile_ = interfaceOut.Get();
        }
    }
    catch (args::Help &e) {
        std::cout << parser;
        throw CompilationCancellation();
    }
    catch (args::ParseError &e) {
        printCliMessage(e.what());
        std::cerr << parser;
        throw CompilationCancellation();
    }
    catch (args::ValidationError &e) {
        printCliMessage(e.what());
        throw CompilationCancellation();
    }

    examineMainFile();
    configureOutPath();
}

void Options::readEnvironment() {
    llvm::SmallString<8> packages("packages");
    llvm::sys::fs::make_absolute(packages);
    packageSearchPaths_.emplace_back(packages.c_str());

    if (const char *ppath = getenv("EMOJICODE_PACKAGES_PATH")) {
        packageSearchPaths_.emplace_back(ppath);
    }

    packageSearchPaths_.emplace_back(defaultPackagesDirectory);
}

void Options::printCliMessage(const std::string &message) {
    std::cout << "👉  " << message << std::endl;
}

void Options::examineMainFile() {
    if (endsWith(mainFile_, ".emojimig")) {
        migrationFile_ = mainFile_;
        format_ = true;
        mainFile_.replace(mainFile_.size() - 8, 8, "emojic");
    }
}

void Options::configureOutPath() {
    if (outPath_.empty()) {
        outPath_ = mainFile_;
        if (linkToExec()) {
            outPath_.resize(mainFile_.size() - 7);
        }
        else {
            outPath_.replace(mainFile_.size() - 6, 6, "o");
        }
    }

    if (!linkToExec_ && interfaceFile_.empty()) {
        std::string parentPath = llvm::sys::path::parent_path(mainFile_);
        interfaceFile_ = parentPath + "/" + "interface.emojii";
    }
}

std::unique_ptr<CompilerDelegate> Options::compilerDelegate() const {
    if (jsonOutput_) {
        return std::make_unique<JSONCompilerDelegate>();
    }
    return std::make_unique<HRFCompilerDelegate>(forceColor_);
}

std::string Options::linker() const {
    if (auto var = getenv("CXX")) {
        return var;
    }
    return "c++";
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
