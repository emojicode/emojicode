//
//  CodeGenerator.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/09/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CodeGenerator.hpp"
#include "CallableScoper.hpp"
#include "Class.hpp"
#include "EmojicodeCompiler.hpp"
#include "FunctionPAG.hpp"
#include "Protocol.hpp"
#include "StringPool.hpp"
#include "TypeDefinitionFunctional.hpp"
#include "ValueType.hpp"
#include "MigWriter.hpp"
#include <cstring>
#include <vector>

namespace EmojicodeCompiler {

template <typename T>
void compileUnused(const std::vector<T *> &functions, MigWriter *migWriter) {
    for (auto function : functions) {
        if (!function->used() && !function->isNative()) {
            generateCodeForFunction(function, function->writer_, migWriter);
        }
    }
}

void generateCodeForFunction(Function *function, FunctionWriter &w, MigWriter *migWriter) {
    CallableScoper scoper = CallableScoper();
    if (FunctionPAG::hasInstanceScope(function->compilationMode())) {
        scoper = CallableScoper(&function->owningType().typeDefinitionFunctional()->instanceScope());
    }
    FunctionPAG(*function, function->owningType().disableSelfResolving(), w, scoper, migWriter).compile();
    migWriter->add(function);
}

void generateCode(MigWriter *migWriter) {
    auto &theStringPool = StringPool::theStringPool();
    theStringPool.poolString(EmojicodeString());

    Function::start->setVtiProvider(&Function::pureFunctionsProvider);
    Function::start->vtiForUse();

    for (auto vt : ValueType::valueTypes()) {  // Must be processed first, different sizes
        vt->finalize();
    }
    for (auto eclass : Class::classes()) {  // Can be processed afterwards, all pointers are 1 word
        eclass->finalize();
    }

    while (!Function::compilationQueue.empty()) {
        Function *function = Function::compilationQueue.front();
        generateCodeForFunction(function, function->writer_, migWriter);
        Function::compilationQueue.pop();
    }

    for (auto eclass : Class::classes()) {
        compileUnused(eclass->methodList(), migWriter);
        compileUnused(eclass->initializerList(), migWriter);
        compileUnused(eclass->typeMethodList(), migWriter);
    }

    for (auto vt : ValueType::valueTypes()) {
        compileUnused(vt->methodList(), migWriter);
        compileUnused(vt->initializerList(), migWriter);
        compileUnused(vt->typeMethodList(), migWriter);
    }
    migWriter->finish();
}

}  // namespace EmojicodeCompiler
