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
    args::ArgumentParser parser("Emojicode Compiler 1.0 beta 1. Visit https://www.emojicode.org for help.");
    args::Positional<std::string> file(parser, "file", "The main file of the package to be compiled", std::string(),
                                       args::Options::Required);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> package(parser, "package", "The name of the package", {'p'});
    args::ValueFlag<std::string> out(parser, "out", "Set output path for binary or assembly", {'o'});
    args::ValueFlag<std::string> interfaceOut(parser, "interface", "Output interface to given path", {'i'});
    args::Flag report(parser, "report", "Generate a JSON report about the package", {'r'});
    args::Flag object(parser, "object", "Produce object file, do not link", {'c'});
    args::Flag json(parser, "json", "Show compiler messages as JSON", {"json"});
    args::Flag format(parser, "format", "Format source code", {"format"});
    args::Flag color(parser, "color", "Always show compiler messages in color", {"color"});
    args::Flag optimize(parser, "optimize", "Compile with optimizations", {'O'});
    args::Flag printIr(parser, "emit-llvm", "Print the IR to the standard output", {"emit-llvm"});
    args::ValueFlagList<std::string> searchPaths(parser, "search path",
                                                 "Adds the path to the package search path (after './packages')",
                                                 {'S'});

    try {
        parser.ParseCLI(argc, argv);

        readEnvironment(searchPaths.Get());

        report_ = report.Get();
        mainFile_ = file.Get();
        jsonOutput_ = json.Get();
        format_ = format.Get();
        forceColor_ = color.Get();
        optimize_ = optimize.Get();
        printIr_ = printIr.Get();

        if (package) {
            mainPackageName_ = package.Get();
        }
        if (object || printIr_) {
            pack_ = false;
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

    configureOutPath();
}

void Options::readEnvironment(const std::vector<std::string> &searchPaths) {
    llvm::SmallString<8> packages("packages");
    llvm::sys::fs::make_absolute(packages);
    packageSearchPaths_.emplace_back(packages.c_str());

    packageSearchPaths_.insert(packageSearchPaths_.begin(), searchPaths.begin(), searchPaths.end());

    if (const char *ppath = getenv("EMOJICODE_PACKAGES_PATH")) {
        packageSearchPaths_.emplace_back(ppath);
    }

    packageSearchPaths_.emplace_back(defaultPackagesDirectory);
}

void Options::printCliMessage(const std::string &message) {
    std::cout << "👉  " << message << std::endl;
}

void Options::configureOutPath() {
    outDir_ = outPath_.empty() ? llvm::sys::path::parent_path(mainFile_) :
                                 llvm::sys::path::parent_path(outPath_);
    if (!outDir_.empty()) {
        outDir_.append("/");
    }

    if (pack() && outPath_.empty()) {
        if (standalone()) {
            outPath_ = outDir_ + std::string(llvm::sys::path::stem(mainFile_));
        }
        else {
            outPath_ = outDir_ + "lib" + mainPackageName_ + ".a";
        }
    }

    if (!standalone() && interfaceFile_.empty()) {
        interfaceFile_ = outDir_ + "🏛";
    }

    if (shouldReport()) {
        reportPath_ = outDir_ + "documentation.json";
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

std::string Options::ar() const {
    if (auto var = getenv("AR")) {
        return var;
    }
    return "ar";
}

std::string Options::objectPath() const {
    if (!pack() && !outPath_.empty()) {
        return outPath_;
    }
    return outDir_ + std::string(llvm::sys::path::stem(mainFile_)) + ".o";
}

std::string Options::llvmIrPath() const {
    if (!printIr_) {
        return "";
    }
    return outDir_ + std::string(llvm::sys::path::stem(mainFile_)) + ".ll";
}

}  // namespace CLI

}  // namespace EmojicodeCompiler
