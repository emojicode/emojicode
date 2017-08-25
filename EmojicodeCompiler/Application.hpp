//
//  Application.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 24/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp

#include "Types/Type.hpp"
#include "Generation/StringPool.hpp"
#include <vector>
#include <map>
#include <queue>

namespace EmojicodeCompiler {

class Package;
class Function;

/// The application class represents an Emojicode application and manages all steps necessary to transform
/// source code documents into an Emojicode binary.
/// Instance of this class own all Packages associated with them.
class Application {
public:
    Application(std::string mainFile, std::string outPath, std::string pkgDir) : mainFile_(std::move(mainFile)),
    outPath_(std::move(outPath)), packageDirectory_(std::move(pkgDir)) {}
    /// Compile the application.
    /// @returns True iff the application has been successfully compiled and an Emojicode binary was written.
    bool compile();

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
    const std::vector<Package *>& packagesInOrder() const { return packagesLoadingOrder_; };
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
    /// Contains all packages in the order in which they must be loaded.
    /// The loading order is only necessary due to class inheritance, so that superclasses are available when the
    /// subclass is read.
    std::vector<Package *> packagesLoadingOrder_;
    std::map<std::string, Package *> packages_;
    std::vector<std::vector<ObjectVariableInformation>> boxObjectVariableInformation_ = std::vector<std::vector<ObjectVariableInformation>>(3);
    Function *startFlag_ = nullptr;
    bool hasError_ = false;
    unsigned int classes_ = 0;
    unsigned int protocols_ = 0;
    const std::string mainFile_;
    const std::string outPath_;
    const std::string packageDirectory_;
    StringPool stringPool_;
};

}  // namespace EmojicodeCompiler


#endif /* Application_hpp */
