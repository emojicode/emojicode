//
//  Compiler.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Compiler_hpp
#define Compiler_hpp

#include "Utils/StringUtils.hpp"
#include "Lex/SourceManager.hpp"
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace EmojicodeCompiler {

class CompilerError;
class RecordingPackage;
class Package;
class Function;
struct SourcePosition;
class Class;
class Protocol;
class ValueType;
class CompatibilityInfoProvider;
class Compiler;

/// CompilerDelegate is an interface class, which is used by Compiler to notify about certain events, like
/// compiler errors.
class CompilerDelegate {
public:
    /// Called when the compilation begins, i.e. when Compiler::compile was called.
    virtual void begin() = 0;
    /// A compiler error occured.
    /// @param p The location at which the error occurred.
    /// @param message A string message describing the error.
    virtual void error(Compiler *compiler, const std::string &message, const SourcePosition &p) = 0;
    /// A compiler warning has been issued.
    /// @param p The location at which the warning was issued.
    /// @param message A string message describing the warning.
    virtual void warn(Compiler *compiler, const std::string &message, const SourcePosition &p) = 0;
    /// Called when the compilation stops, i.e. just before Compiler::compile returns.
    virtual void finish() = 0;
};

/// The Compiler class is the main interface to the compiler.
///
/// It manages the loading of packages, parses and optionally analyses and generates LLVM IR for the main package,
/// emits it as an object file.
class Compiler final {
public:
    /// Constructs an Compiler instance.
    /// @param mainPackage The name of the main package. This is the package for which the compiler will produce
    ///                    an object file and/or an interface.
    /// @param mainFile The main package’s main file. (See Package::Package.)
    /// @param interfaceFile The path at which an interface file for the main package shall be created. Pass an empty
    ///                      string to prevent the creation of an interface file.
    /// @param linker The name or path of an executable that can be used to link object files and static libraries.
    /// @param pkgSearchPaths The paths the compiler will search for a requested package.
    /// @param linkToExec If true an executable is written to outPath, if false an object file representing the package
    ///                   is written to outPath.
    Compiler(std::string mainPackage, std::string mainFile, std::string interfaceFile, std::string outPath,
             std::string linker, std::vector<std::string> pkgSearchPaths, std::unique_ptr<CompilerDelegate> delegate,
             bool linkToExec);
    /// Compile the application.
    /// @param parseOnly If this argument is true, the main package is only parsed and not semantically analysed.
    /// @returns True iff the application has been successfully parsed and — optionally — analysed.
    bool compile(bool parseOnly);

    RecordingPackage* mainPackage() const { return mainPackage_.get(); }

    SourceManager& sourceManager() { return sourceManager_; }

    void loadMigrationFile(const std::string &file);

    /// Issues a compiler warning. The compilation is continued normally.
    /// @param args All arguments will be concatenated.
    template<typename... Args>
    void warn(const SourcePosition &p, Args... args) {
        std::stringstream stream;
        appendToStream(stream, args...);
        warn(p, stream.str());
    }
    /// Issues a compiler warning. The compilation is continued normally.
    void warn(const SourcePosition &p, const std::string &warning);
    /// Issues a compiler error. The compilation can continue, but no code will be generated.
    void error(const CompilerError &ce);

    /// Loads the package with the given name. If the package has already been loaded it is returned immediately.
    /// @param requestor The package that caused the call to this method.
    /// @see findPackage()
    Package* loadPackage(const std::string &name, const SourcePosition &p, Package *requestor);

    void assignSTypes(Package *s, const SourcePosition &errorPosition);

    Class *sString = nullptr;
    Class *sList = nullptr;
    Class *sData = nullptr;
    Class *sDictionary = nullptr;
    Protocol *sEnumerator = nullptr;
    Protocol *sEnumerable = nullptr;
    ValueType *sBoolean = nullptr;
    ValueType *sSymbol = nullptr;
    ValueType *sInteger = nullptr;
    ValueType *sReal = nullptr;
    ValueType *sMemory = nullptr;
    ValueType *sByte = nullptr;

    ~Compiler();

private:
    void generateCode();
    void analyse();
    void linkToExecutable();
    std::string objectFileName() const;
    std::string searchPackage(const std::string &name, const SourcePosition &p);
    std::string findBinaryPathPackage(const std::string &packagePath, const std::string &packageName);

    /// Searches the loaded packages for the package with the given name.
    /// If the package has not been loaded yet @c nullptr is returned.
    Package* findPackage(const std::string &name) const;

    std::map<std::string, std::unique_ptr<Package>> packages_;

    bool hasError_ = false;
    bool linkToExec_;
    std::string mainFile_;
    std::string interfaceFile_;
    const std::string outPath_;
    const std::string mainPackageName_;
    const std::vector<std::string> packageSearchPaths_;
    const std::string linker_;
    const std::unique_ptr<CompilerDelegate> delegate_;
    std::unique_ptr<CompatibilityInfoProvider> compInfoProvider_;
    std::unique_ptr<RecordingPackage> mainPackage_;
    SourceManager sourceManager_;
};

}  // namespace EmojicodeCompiler


#endif /* Compiler_hpp */
