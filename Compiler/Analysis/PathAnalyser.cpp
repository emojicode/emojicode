//
//  PathAnalyser.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 04/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "PathAnalyser.hpp"
#include "Scoping/SemanticScoper.hpp"
#include <algorithm>
#include <cassert>

namespace EmojicodeCompiler {

void PathAnalyser::uninitalizedError(const ResolvedVariable &rvar, const SourcePosition &p) const {
    auto incident = PathAnalyserIncident(rvar.inInstanceScope, rvar.variable.id());
    if (!hasCertainly(incident)) {
        if (hasPotentially(incident)) {
            throw CompilerError(p, "Variable \"", utf8(rvar.variable.name()),"\" is not initialized on all paths.");
        }
        throw CompilerError(p, "Variable \"", utf8(rvar.variable.name()),"\" is not initialized.");
    }
}

void PathAnalyser::copyCertainIncidents() {
    auto incs = currentBranch_->branches[0].certainIncidents;
    for (auto it = currentBranch_->branches.begin() + 1; it < currentBranch_->branches.end(); it++) {
        auto branch = *it;
        auto newIncidents = std::set<PathAnalyserIncident>();
        std::set_intersection(incs.begin(), incs.end(), branch.certainIncidents.begin(),
                              branch.certainIncidents.end(), std::inserter(newIncidents, newIncidents.begin()));
        incs = newIncidents;
    }
    currentBranch_->certainIncidents.insert(incs.begin(), incs.end());
}

bool PathAnalyser::hasCertainly(PathAnalyserIncident incident) const {
    for (auto branch = currentBranch_; branch != nullptr; branch = branch->parent) {
        if (branch->certainIncidents.find(incident) != branch->certainIncidents.end()) {
            return true;
        }
    }
    return false;
}

bool PathAnalyser::hasPotentially(PathAnalyserIncident incident) const {
    for (auto branch = currentBranch_; branch != nullptr; branch = branch->parent) {
        if (branch->potentialIncidents.find(incident) != branch->potentialIncidents.end()) {
            return true;
        }
    }
    return false;
}

void PathAnalyser::copyPotentialIncidents() {
    for (auto branch : currentBranch_->branches) {
        currentBranch_->potentialIncidents.insert(branch.potentialIncidents.begin(),
                                                  branch.potentialIncidents.end());
    }
}

void PathAnalyser::finishMutualExclusiveBranches() {
    if (currentBranch_->branches.empty()) {
        return;
    }

    copyPotentialIncidents();
    copyCertainIncidents();

    currentBranch_->branches.clear();
}

void PathAnalyser::endBranch() {
    assert(currentBranch_->branches.empty());
    auto parent = currentBranch_->parent;
    currentBranch_->parent = nullptr;
    currentBranch_ = parent;
}

}  // namespace EmojicodeCompiler
