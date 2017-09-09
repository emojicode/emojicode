//
//  Application.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp

#include "Package/Package.hpp"
#include "Parsing/CompatibilityInfoProvider.hpp"
#include "Types/Type.hpp"
#include <exception>
#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace EmojicodeCompiler {

class Package;
class Function;
struct SourcePosition;

/// ApplicationDelegate is an interface class, which is used by Application to notify about certain events, like
/// compiler errors.
class ApplicationDelegate {
public:
    /// Called when the compilation begins, i.e. when Application::compile was called.
    virtual void begin() = 0;
    /// A compiler error occured.
    /// @param p The location at which the error occurred.
    /// @param message A string message describing the error.
    virtual void error(const SourcePosition &p, const std::string &message) = 0;
    /// A compiler warning has been issued.
    /// @param p The location at which the warning was issued.
    /// @param message A string message describing the warning.
    virtual void warn(const SourcePosition &p, const std::string &message) = 0;
    /// Called when the compilation stops, i.e. just before Application::compile returns.
    virtual void finish() = 0;
};

/// The application class represents an Emojicode application and manages all steps necessary to transform
/// source code documents into an Emojicode binary.
/// Instance of this class own all Packages associated with them.
class Application final {
public:
    /// Constructs an Application instance.
    /// @param mainPackage The name of the main package. This is the package for which the compiler will produce
    ///                    an object file if compile(false) is called. It's also the file whose interface will be
    ///                    created.
    /// @param mainFile The main package’s main file. (See Package::Package.)
    /// @param outPath The path at which the Emojicode Binary will be written.
    /// @param pkgDir The path to the Emojicode packages directory. It will be searched if a package is loaded
    ///               via loadPackage().
    Application(std::string mainPackage, std::string mainFile, std::string outPath, std::string pkgDir,
                std::unique_ptr<ApplicationDelegate> delegate, bool standalone);
    /// Compile the application.
    /// @param parseOnly If this argument is true, all packages are only parsed and not semantically analysed.
    /// @returns True iff the application has been successfully compiled and an Emojicode binary was written.
    bool compile(bool parseOnly);

    /// Constructs a Package instance that represents the main package. Appropriate values are set.
    template <typename T>
    void factorMainPackage() {
        if (mainPackage_ == nullptr) {
            mainPackage_ = std::make_unique<T>(std::move(mainPackageName_), mainFile_, this);
            if (standalone_) {
                mainPackage_->setPackageVersion(PackageVersion(1, 0));
            }
            mainPackage_->setCompatiblityInfoProvider(compInfoProvider_.get());
        }
    }

    Package* mainPackage() const { return mainPackage_.get(); }

    /// @pre factorUnderscorePackage() must not have been called.
    void loadMigrationFile(const std::string &file) {
        if (mainPackage_ != nullptr) {
            throw std::logic_error("loadMigrationFile must not be called after factorUnderscorePackage");
        }
        compInfoProvider_ = std::make_unique<CompatibilityInfoProvider>(file);
    }

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

    void setStartFlagFunction(Function *function) { startFlag_ = function; }
    Function* startFlagFunction() const { return startFlag_; }
    /// Whether a start flag function was encountered and compiled.
    bool hasStartFlagFunction() const { return startFlag_ != nullptr; }

    /// Searches the loaded packages for the package with the given name.
    /// If the package has not been loaded yet @c nullptr is returned.
    Package* findPackage(const std::string &name) const;
    /// Loads the package with the given name. If the package has already been loaded it is returned immediately.
    /// @param requestor The package that caused the call to this method.
    /// @see findPackage()
    Package* loadPackage(const std::string &name, const SourcePosition &p, Package *requestor);

    std::queue<Function *> analysisQueue;

    const std::vector<std::vector<ObjectVariableInformation>>& boxObjectVariableInformation() const {
        return boxObjectVariableInformation_;
    }
    std::vector<std::vector<ObjectVariableInformation>>& boxObjectVariableInformation() {
        return boxObjectVariableInformation_;
    }
private:
    void generateCode();
    void analyse(Package *underscorePackage);

    /// Contains all packages in the order in which they must be loaded.
    /// The loading order is only necessary due to class inheritance, so that superclasses are available when the
    /// subclass is read.
    std::vector<std::unique_ptr<Package>> packagesLoadingOrder_;
    std::map<std::string, Package *> packages_;
    std::vector<std::vector<ObjectVariableInformation>> boxObjectVariableInformation_ = std::vector<std::vector<ObjectVariableInformation>>(3);
    Function *startFlag_ = nullptr;
    bool hasError_ = false;
    bool standalone_;
    std::string mainFile_;
    const std::string outPath_;
    const std::string mainPackageName_;
    const std::string packageDirectory_;
    std::unique_ptr<ApplicationDelegate> delegate_;
    std::unique_ptr<CompatibilityInfoProvider> compInfoProvider_;
    std::unique_ptr<Package> mainPackage_;
};

}  // namespace EmojicodeCompiler


#endif /* Application_hpp */
