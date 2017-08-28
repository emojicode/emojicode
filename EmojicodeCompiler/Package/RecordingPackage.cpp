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
        packages_.emplace_back(name, ns);
    }
    Package::importPackage(name, ns, p);
}

void RecordingPackage::offerType(Type t, const std::u32string &name, const std::u32string &ns, bool exportFromPkg,
                                 const SourcePosition &p) {
    if (exportFromPkg || t.typeDefinition()->package() == this) {
        types_.emplace_back(t);
    }
    Package::offerType(std::move(t), name, ns, exportFromPkg, p);
}

};
