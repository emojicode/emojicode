//
//  Extension.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 06/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Extension.hpp"
#include "Functions/Function.hpp"
#include <functional>

namespace EmojicodeCompiler {

void Extension::extend() {
    if (extendedType_.typeDefinition()->package() != package_) {
        throw CompilerError(position(), "Types from other package cannot be extended at this time.");
    }

    auto typeDef = extendedType_.typeDefinition();
    for (auto &method : methods_) {
        typeDef->addMethod(std::move(method.second));
    }
    for (auto &initializer : initializers_) {
        typeDef->addInitializer(std::move(initializer.second));
    }
    for (auto &method : typeMethods_) {
        typeDef->addTypeMethod(std::move(method.second));
    }
    for (auto &protocol : protocols_) {
        typeDef->addProtocol(protocol);
    }
    for (auto &var : instanceVariables_) {
        typeDef->addInstanceVariable(var);
    }
}

}  // namespace EmojicodeCompiler
