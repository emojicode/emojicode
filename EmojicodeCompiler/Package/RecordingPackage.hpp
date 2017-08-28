//
//  RecordingPackage.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 26/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef RecordingPackage_hpp
#define RecordingPackage_hpp

#include "Package.hpp"

namespace EmojicodeCompiler {

/// The RecordingPackage class records all packages that it’s imported as well as all types offered that were
/// not offered as a result of a package import.
class RecordingPackage : public Package {
    using Package::Package;
public:
    struct RecordedImport {
        RecordedImport(std::string pkg, std::u32string ns)
        : package(std::move(pkg)), destNamespace(std::move(ns)) {}
        std::string package;
        std::u32string destNamespace;
    };

    /// @returns A vector of Type instances which represent the type definitions.
    const std::vector<Type>& types() const { return types_; }
    /// @returns A vector of RecordedImport instances that represent all package imports in the order they occurred.
    const std::vector<RecordedImport>& importedPackages() const { return packages_; }

    void importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) override;
    void offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                           const SourcePosition &p) override;
    Extension& registerExtension(Extension ext) override;
private:
    std::vector<Type> types_;
    std::vector<RecordedImport> packages_;
};

}

#endif /* RecordingPackage_hpp */
