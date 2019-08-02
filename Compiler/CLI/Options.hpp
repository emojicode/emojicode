//
//  Options.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Options_hpp
#define Options_hpp

#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace EmojicodeCompiler {

class CompilerDelegate;

/// This namespace contains all types and functions that define the command-line interface of the Emojicode Compiler.
namespace CLI {

/// Thrown by Options if the cancellation should not begin.
class CompilationCancellation : public std::exception {};

/// An instance of this class represents the command-line options with which the compiler was started.
class Options {
public:
    /// Constructs an Options instance from the command-line arguments and environment variables.
    /// @throws CompilationCancellation
    Options(int argc, char *argv[]);

    /// This method must be used to print messages or errors about the command-line interface use.
    void printCliMessage(const std::string &message);

    /// @returns A CompilerDelegate that matches the options represented by the instance.
    std::unique_ptr<CompilerDelegate> compilerDelegate() const;

    bool shouldReport() const { return report_; }
    bool optimize() const { return optimize_; }
    bool pack() const { return pack_; }
    bool standalone() const { return mainPackageName_ == "_"; }

    const std::string& outPath() const { return outPath_; }
    const std::string& mainFile() const { return mainFile_; }
    const std::string& interfaceFile() const { return interfaceFile_; }
    const std::vector<std::string>& packageSearchPaths() const { return packageSearchPaths_; }
    const std::string& mainPackageName() const { return mainPackageName_; }
    const std::string& reportPath() const { return reportPath_; }
    std::string llvmIrPath() const;
    std::string linker() const;
    std::string ar() const;

    /// Whether the main purpose of the invocation of the compiler is to prettyprint a file.
    /// This method returns true if prettyprint was explicitely requested or if a file is being migrated.
    bool prettyprint() const { return format_; }

    std::string objectPath() const;
private:
    std::string outPath_;
    std::string mainFile_;
    std::string interfaceFile_;
    std::string reportPath_;
    std::string llvmIr_;
    std::vector<std::string> packageSearchPaths_;
    std::string mainPackageName_ = "_";
    /// Path to the directory where the output files will be placed.
    std::string outDir_;
    bool format_ = false;
    bool jsonOutput_ = false;
    bool pack_ = true;
    bool report_ = false;
    bool forceColor_ = false;
    bool optimize_ = false;
    bool printIr_ = false;

    void readEnvironment(const std::vector<std::string> &searchPaths);

    void configureOutPath();
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* Options_hpp */
