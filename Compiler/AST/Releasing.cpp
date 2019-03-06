//
//  Releasing.cpp
//  runtime
//
//  Created by Theo Weidmann on 05.03.19.
//

#include "Releasing.hpp"
#include "Generation/FunctionCodeGenerator.hpp"
#include "ASTMemory.hpp"

namespace EmojicodeCompiler {

Releasing::Releasing() = default;
Releasing::~Releasing() = default;

void Releasing::addRelease(std::unique_ptr<ASTRelease> release) { releases_.emplace_back(std::move(release)); }

void Releasing::release(FunctionCodeGenerator *fg) const {
    for (auto &release : releases_) {
        release->generate(fg);
    }
}

}
