//
//  CompatibilityInfoProvider.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "CompatibilityInfoProvider.hpp"
#include "../Functions/Function.hpp"
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
    selectedFunction_ = &files_[function->position().file][function->position().line];
    index_ = 0;
}

unsigned int CompatibilityInfoProvider::nextArgumentsCount() {
    if (index_ == selectedFunction_->size()) {
        throw "error";
    }
    return (*selectedFunction_)[index_++];
}


}  // namespace EmojicodeCompiler
