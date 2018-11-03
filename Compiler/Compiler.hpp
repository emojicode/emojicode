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
#include <map>
#include <memory>
#include <string>
#include <vector>

/// The main namespace of the Emojicode Compiler. It contains everything releated to compilation.
namespace EmojicodeCompiler {

class CompilerError;
class RecordingPackage;
class Package;
class Function;
struct SourcePosition;
class Class;
class Protocol;
class ValueType;
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

    virtual ~CompilerDelegate() = default;
};

/// The Compiler class is the main interface to the compiler.
///
/// It manages the loading of packages, parses and optionally analyses and generates LLVM IR for the main package,
/// emits it as an object file.
class Compiler final {
public:
    /// Constructs an Compiler instance.
    /// @param mainPackage The name of the main package. This is the package for which the compiler can produce
    ///                    an object file, interface, executable and/or archive.
    /// @param mainFile The main package’s main file. (See Package::Package.)
    /// @param interfaceFile The path at which an interface file for the main package shall be created. Pass an empty
    ///                      string to prevent the creation of an interface file.
    /// @param linker The name or path of an executable that can be used to link object files and static libraries.
    /// @param pkgSearchPaths The paths the compiler will search for a requested package.
    /// @param standalone Whether the package is a package to be imported into another package or a standalone program.
    ///                   The latter requires no version and a start flag block.
    /// @param pack Whether an executable/archive should be created.
    /// @param objectPath The path at which the object file will be placed.
    /// @param outPath The path at which the ‘packed’ output (executable/archive) will be placed.
    Compiler(std::string mainPackage, std::string mainFile, std::string interfaceFile, std::string outPath,
             std::string objectPath, std::string linker, std::string ar, std::vector<std::string> pkgSearchPaths,
             std::unique_ptr<CompilerDelegate> delegate, bool pack, bool standalone);
    /// Compile the application.
    /// @param parseOnly If this argument is true, the main package is only parsed and not semantically analysed.
    /// @returns True iff the application has been successfully parsed and — optionally — analysed.
    bool compile(bool parseOnly, bool optimize, bool printIr);

    RecordingPackage *mainPackage() const { return mainPackage_.get(); }

    SourceManager &sourceManager() { return sourceManager_; }

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
    Package *loadPackage(const std::string &name, const SourcePosition &p, Package *requestor);

    void assignSTypes(Package *s);

    Class *sString = nullptr;
    Class *sList = nullptr;
    Class *sDictionary = nullptr;
    Protocol *sEnumerable = nullptr;
    ValueType *sBoolean = nullptr;
    ValueType *sInteger = nullptr;
    ValueType *sReal = nullptr;
    ValueType *sMemory = nullptr;
    ValueType *sByte = nullptr;

    ~Compiler();

private:
    void generateCode(bool optimize, bool printIr);
    void analyse();
    void linkToExecutable();
    std::string searchPackage(const std::string &name, const SourcePosition &p);
    std::string findBinaryPathPackage(const std::string &packagePath, const std::string &packageName);

    /// Searches the loaded packages for the package with the given name.
    /// If the package has not been loaded yet @c nullptr is returned.
    Package *findPackage(const std::string &name) const;

    std::map<std::string, std::unique_ptr<Package>> packages_;

    bool hasError_ = false;
    bool pack_;
    bool standalone_;
    std::string mainFile_;
    std::string interfaceFile_;
    const std::string outPath_;
    const std::string mainPackageName_;
    const std::vector<std::string> packageSearchPaths_;
    const std::string linker_;
    const std::string ar_;
    const std::string objectPath_;
    const std::unique_ptr<CompilerDelegate> delegate_;
    std::unique_ptr<RecordingPackage> mainPackage_;
    SourceManager sourceManager_;
    void archive();
};

}  // namespace EmojicodeCompiler


#endif /* Compiler_hpp */
