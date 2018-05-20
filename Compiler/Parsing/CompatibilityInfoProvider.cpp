//
//  CompatibilityInfoProvider.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CompatibilityInfoProvider.hpp"
#include "CompilerError.hpp"
#include "Functions/Function.hpp"
#include "Lex/SourceManager.hpp"
#include <fstream>
#include <iostream>

namespace EmojicodeCompiler {

CompatibilityInfoProvider::CompatibilityInfoProvider(const std::string &path) {
    auto file = std::fstream(path, std::ios_base::in);
    while (file.peek() == '-') {
        file.get();
        std::string path;
        std::getline(file, path);
        auto &function = files_.emplace(path, std::map<size_t, std::vector<unsigned int>>()).first->second;
        while (file.peek() == '#') {
            file.get();
            size_t line;
            file >> line;
            file.get();  // consume a line break
            auto &counts = function.emplace(line, std::vector<unsigned int>()).first->second;
            while (file.peek() != '\n') {
                unsigned int count;
                file >> count;
                counts.emplace_back(count);
                if (file.peek() == ' ') {
                    file.get();  // consume trailing whitespace to properly detect line break
                }
            }
            file.get();  // consume line break
        }
    }
}

void CompatibilityInfoProvider::selectFunction(Function *function) {
    selection_.selectedFunction_ = &files_[function->position().file->path()][function->position().line];
    selection_.index_ = 0;
}

unsigned int CompatibilityInfoProvider::nextArgumentsCount(const SourcePosition &p) {
    if (selection_.index_ == selection_.selectedFunction_->size()) {
        throw CompilerError(p, "Migration file does not contain enough argument counts. Either the migration file "\
                            "was corrupted or the compiler classified an identifer as method call where it shouldn’t. "\
                            "Please see the caveats.");
    }
    return (*selection_.selectedFunction_)[selection_.index_++];
}


}  // namespace EmojicodeCompiler
