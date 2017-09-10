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
#include <memory>

namespace EmojicodeCompiler {

/// The RecordingPackage class records all packages that it’s imported as well as all types offered that were
/// not offered as a result of a package import.
class RecordingPackage : public Package {
    using Package::Package;
public:
    class Recording {
    public:
        virtual ~Recording() = default;
    };

    class Import : public Recording {
    public:
        Import(std::string pkg, std::u32string ns)
        : package(std::move(pkg)), destNamespace(std::move(ns)) {}
        std::string package;
        std::u32string destNamespace;
    };

    class Include : public Recording {
    public:
        explicit Include(std::string path) : path_(std::move(path)) {}
        std::string path_;
    };

    class RecordedType : public Recording {
    public:
        explicit RecordedType(Type type) : type_(std::move(type)) {}
        Type type_;
    };

    struct File {
        explicit File(std::string path) : path_(std::move(path)) {}
        std::string path_;
        std::vector<std::unique_ptr<Recording>> recordings_;
    };

    const std::vector<File>& files() const { return files_; }

    void importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) override;
    void offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                           const SourcePosition &p) override;
    Extension* add(std::unique_ptr<Extension> &&ext) override;
    void includeDocument(const std::string &path, const std::string &relativePath) override;
private:
    std::vector<File> files_;
    size_t currentFile_ = 0;
};

}  // namespace EmojicodeCompiler

#endif /* RecordingPackage_hpp */
