//
//  Application.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp

#include "Generation/StringPool.hpp"
#include "Types/Type.hpp"
#include "Package/Package.hpp"
#include "Parsing/CompatibilityInfoProvider.hpp"
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <exception>

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
    /// @param mainFile The underscore package’s main file. (See Package::Package.)
    /// @param outPath The path at which the Emojicode Binary will be written.
    /// @param pkgDir The path to the Emojicode packages directory. It will be searched if a package is loaded
    ///               via loadPackage().
    Application(std::string mainFile, std::string outPath, std::string pkgDir,
                std::unique_ptr<ApplicationDelegate> delegate);
    /// Compile the application.
    /// @param parseOnly If this argument is true, all packages are only parsed and not semantically analysed.
    /// @returns True iff the application has been successfully compiled and an Emojicode binary was written.
    bool compile(bool parseOnly);

    /// Constructs a Package instance that represents the underscore package. Appropriate values are set.
    template <typename T>
    void factorUnderscorePackage() {
        if (underscorePackage() == nullptr) {
            underscorePackage_ = std::make_unique<T>("_", mainFile_, this);
            underscorePackage_->setPackageVersion(PackageVersion(1, 0));
            underscorePackage_->setRequiresBinary(false);
            underscorePackage_->setCompatiblityInfoProvider(compInfoProvider_.get());
        }
    }

    /// Returns the underscore package.
    /// If the underscore package has not been factored yet, nullptr is returned.
    Package* underscorePackage() {
        if (underscorePackage_ != nullptr) {
            return underscorePackage_.get();
        }
        if (!packagesLoadingOrder_.empty()) {
            return packagesLoadingOrder_.back().get();
        }
        return nullptr;
    }

    /// @pre factorUnderscorePackage() must not have been called.
    void loadMigrationFile(const std::string &file) {
        if (underscorePackage() != nullptr) {
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

    /// Returns all packages in the order in which they were loaded.
    const std::vector<std::unique_ptr<Package>>& packagesInOrder() const { return packagesLoadingOrder_; }
    /// Searches the loaded packages for the package with the given name.
    /// If the package has not been loaded yet @c nullptr is returned.
    Package* findPackage(const std::string &name) const;
    /// Loads the package with the given name. If the package has already been loaded it is returned immediately.
    /// @param requestor The package that caused the call to this method.
    /// @see findPackage()
    Package* loadPackage(const std::string &name, const SourcePosition &p, Package *requestor);

    /// Returns this application’s string pool
    StringPool& stringPool() { return stringPool_; }

    std::queue<Function *> compilationQueue;
    std::queue<Function *> analysisQueue;

    unsigned int classIndex() { return classes_++; }
    unsigned int classCount() { return classes_; }

    unsigned int protocolIndex() { return protocols_++; }
    unsigned int protocolCount() { return protocols_; }

    uint32_t maxBoxIndetifier() const { return static_cast<uint32_t>(boxObjectVariableInformation_.size()); }
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
    unsigned int classes_ = 0;
    unsigned int protocols_ = 0;
    std::string mainFile_;
    const std::string outPath_;
    const std::string packageDirectory_;
    StringPool stringPool_;
    std::unique_ptr<ApplicationDelegate> delegate_;
    std::unique_ptr<CompatibilityInfoProvider> compInfoProvider_;
    std::unique_ptr<Package> underscorePackage_;
};

}  // namespace EmojicodeCompiler


#endif /* Application_hpp */
