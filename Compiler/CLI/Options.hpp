//
//  Options.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 25/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Options_hpp
#define Options_hpp

#include "Compiler.hpp"
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace EmojicodeCompiler {

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
    const std::string& outPath() const { return outPath_; }
    const std::string& mainFile() const { return mainFile_; }
    const std::string& interfaceFile() const { return interfaceFile_; }
    const std::vector<std::string>& packageSearchPaths() const { return packageSearchPaths_; }
    const std::string& migrationFile() const { return migrationFile_; }
    const std::string& mainPackageName() const { return mainPackageName_; }
    std::string linker() const;

    bool linkToExec() const { return linkToExec_; }

    /// Whether the main purpose of the invocation of the compiler is to prettyprint a file.
    /// This method returns true if prettyprint was explicitely requested or if a file is being migrated.
    bool prettyprint() const { return format_; }
private:
    std::string outPath_;
    std::string mainFile_;
    std::string interfaceFile_;
    std::vector<std::string> packageSearchPaths_;
    std::string migrationFile_;
    std::string mainPackageName_ = "_";
    bool format_ = false;
    bool jsonOutput_ = false;
    bool linkToExec_ = true;
    bool report_ = false;

    void readEnvironment();

    /// If the file ends in ".emojimig", the migration file migrationFile_ will be set to it and prettyprint_ to true.
    /// The main file is then derived from by replacing ".emojimig" with ".emojic".
    void examineMainFile();
    void configureOutPath();
};

}  // namespace CLI

}  // namespace EmojicodeCompiler

#endif /* Options_hpp */
