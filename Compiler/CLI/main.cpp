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
#include "Prettyprint/PrettyPrinter.hpp"
#include <exception>
#include <iostream>

namespace EmojicodeCompiler {

namespace CLI {

class FormatPhase : public Compiler::Phase {
    void perform(Compiler *compiler) override {
        PrettyPrinter(compiler->mainPackage()).print();
    }
};

class ReportPhase : public Compiler::Phase {
public:
    ReportPhase(std::string path) : path_(std::move(path)) {}
    void perform(Compiler *compiler) override {
        PackageReporter(compiler->mainPackage(), path_).report();
    }

private:
    std::string path_;
};

/// The compiler CLI main function
/// @returns True if the requested operation was successful.
bool start(const Options &options) {
    Compiler compiler(options.mainPackageName(), options.mainFile(), options.packageSearchPaths(),
                      options.compilerDelegate());

    compiler.add<Compiler::ParsePhase>();
    if (options.prettyprint()) {
        compiler.add<FormatPhase>();
        return compiler.compile();
    }
    compiler.add<Compiler::AnalysisPhase>(options.standalone());
    if (!options.interfaceFile().empty()) {
        compiler.add<Compiler::PrintInterfacePhase>(options.interfaceFile());
    }
    compiler.add<Compiler::GenerationPhase>(options.optimize());
    if (!options.llvmIrPath().empty()) {
        compiler.add<Compiler::LLVMIREmissionPhase>(options.llvmIrPath());
    }
    else {
        compiler.add<Compiler::ObjectFileEmissionPhase>(options.objectPath());
    }
    if (options.pack()) {
        if (options.standalone()) {
            compiler.add<Compiler::LinkPhase>(options.objectPath(), options.outPath(), options.linker());
        }
        else {
            compiler.add<Compiler::ArchivePhase>(options.objectPath(), options.outPath(), options.ar());
        }
    }

    if (options.shouldReport()) {
        compiler.add<ReportPhase>(options.reportPath());
    }
    return compiler.compile();
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
