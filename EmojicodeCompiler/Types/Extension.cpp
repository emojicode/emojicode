//
//  Extension.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Extension.hpp"
#include "../Function.hpp"
#include <functional>

namespace EmojicodeCompiler {

void Extension::extend() {
    auto typeDef = extendedType_.typeDefinition();
    for (auto method : methodList_) {
        typeDef->addMethod(method);
    }
    for (auto initializer : initializerList_) {
        typeDef->addInitializer(initializer);
    }
    for (auto method : typeMethodList_) {
        typeDef->addTypeMethod(method);
    }
    for (auto &protocol : protocols_) {
        typeDef->addProtocol(protocol, position());
        typeDef->finalizeProtocol(extendedType_, protocol, true);
    }
}

}  // namespace EmojicodeCompiler
