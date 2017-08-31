//
//  MigWriter.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 28/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "MigWriter.hpp"
#include "Function.hpp"

namespace EmojicodeCompiler {

void MigWriter::add(Function *function) {
    if (function->package() != underscore_ || function->compilationMode() == FunctionPAGMode::BoxingLayer) {
        return;
    }
    functions_[function->position().file].emplace_back(function);
}

void MigWriter::finish() {
    for (auto &pair : functions_) {
        out_ << "-" << pair.first << "\n";
        for (auto function : pair.second) {
            out_ << "#" << function->position().line << "\n";
            for (auto arg : function->creator.arguments_) {
                out_ << arg << " ";
            }
            out_ << "\n";
        }
    }
    out_.flush();
}

}
