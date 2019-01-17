//
//  PathAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 04/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef PathAnalyser_hpp
#define PathAnalyser_hpp

#include <algorithm>
#include <cassert>
#include <vector>
#include <set>

namespace EmojicodeCompiler {

enum class PathAnalyserIncident {
    CalledSuperInitializer,
    Returned,
    UsedSelf,
};

class PathAnalyser {
    struct Branch {
        explicit Branch(Branch *parent) : parent(parent) {}

        Branch *parent;
        std::set<PathAnalyserIncident> certainIncidents;
        std::set<PathAnalyserIncident> potentialIncidents;
        std::vector<Branch> branches;
    };

public:
    void endMutualExclusiveBranches() {
        if (currentBranch_->branches.empty()) {
            return;
        }

        copyPotentialIncidents();
        copyCertainIncidents();

        currentBranch_->branches.clear();
    }

    void endUncertainBranches() {
        copyPotentialIncidents();
        currentBranch_->branches.clear();
    }

    void recordIncident(PathAnalyserIncident incident) {
        currentBranch_->certainIncidents.emplace(incident);
        currentBranch_->potentialIncidents.emplace(incident);
    }

    void beginBranch() {
        currentBranch_->branches.emplace_back(Branch(currentBranch_));
        currentBranch_ = &currentBranch_->branches.back();
    }

    void endBranch() {
        assert(currentBranch_->branches.empty());
        auto parent = currentBranch_->parent;
        currentBranch_->parent = nullptr;
        currentBranch_ = parent;
    }

    bool hasCertainly(PathAnalyserIncident incident) const {
        return currentBranch_->certainIncidents.find(incident) != currentBranch_->certainIncidents.end();
    }

    bool hasPotentially(PathAnalyserIncident incident) const {
        return currentBranch_->potentialIncidents.find(incident) != currentBranch_->potentialIncidents.end();
    }

private:
    void copyPotentialIncidents() {
        for (auto branch : currentBranch_->branches) {
            currentBranch_->potentialIncidents.insert(branch.potentialIncidents.begin(),
                                                      branch.potentialIncidents.end());
        }
    }

    void copyCertainIncidents() {
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

    Branch mainBranch_ = Branch(nullptr);
    Branch *currentBranch_ = &mainBranch_;
};

}  // namespace EmojicodeCompiler

#endif /* PathAnalyser_hpp */
