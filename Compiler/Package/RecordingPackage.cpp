//
//  RecordingPackage.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 26/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "RecordingPackage.hpp"

namespace EmojicodeCompiler {

void RecordingPackage::importPackage(const std::string &name, const std::u32string &ns, const SourcePosition &p) {
    if (name != "s" || ns != kDefaultNamespace) {
        files_[currentFile_].recordings_.emplace_back(std::make_unique<Import>(name, ns));
    }
    Package::importPackage(name, ns, p);
}

void RecordingPackage::offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                                 const SourcePosition &p) {
    if (exportFromPkg || t.typeDefinition()->package() == this) {
        files_[currentFile_].recordings_.emplace_back(std::make_unique<RecordedType>(t));
    }
    Package::offerType(std::move(t), name, ns, exportFromPkg, p);
}

Extension& RecordingPackage::registerExtension(Extension ext) {
    auto &extension = Package::registerExtension(std::move(ext));
    files_[currentFile_].recordings_.emplace_back(std::make_unique<RecordedType>(Type(&extension)));
    return extension;
}

void RecordingPackage::includeDocument(const std::string &path, const std::string &relativePath) {
    auto temp = currentFile_;
    if (!files_.empty()) {
        files_[currentFile_].recordings_.emplace_back(std::make_unique<Include>(relativePath));
    }
    currentFile_ = files_.size();
    files_.emplace_back(path);
    Package::includeDocument(path, relativePath);
    currentFile_ = temp;
}

}  // namespace EmojicodeCompiler
