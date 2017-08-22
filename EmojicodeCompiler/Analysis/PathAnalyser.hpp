//
//  PathAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 04/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef PathAnalyser_hpp
#define PathAnalyser_hpp

#include "../EmojicodeCompiler.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

namespace EmojicodeCompiler {

enum class PathAnalyserIncident {
    CalledSuperInitializer,
    Returned,
    UsedSelf,
};

class PathAnalyser {
    struct Branch {
        Branch(Branch *parent) : parent(parent) {}
        Branch *parent;
        std::vector<PathAnalyserIncident> certainIncidents;
        std::vector<PathAnalyserIncident> potentialIncidents;
        std::vector<Branch> branches;
    };
public:
    void endMutualExclusiveBranches() {
        if (currentBranch_->branches.empty()) {
            return;
        }
        
        copyPotentialIncidents();
        auto incs = currentBranch_->branches[0].certainIncidents;
        for (auto it = currentBranch_->branches.begin() + 1; it < currentBranch_->branches.end(); it++) {
            auto branch = *it;
            std::set_intersection(incs.begin(), incs.end(), branch.certainIncidents.begin(),
                                  branch.certainIncidents.end(), std::inserter(incs, incs.begin()));
        }
        currentBranch_->certainIncidents.insert(currentBranch_->certainIncidents.begin(), incs.begin(), incs.end());
        currentBranch_->branches.clear();
    }
    void endUncertainBranches() {
        copyPotentialIncidents();
        currentBranch_->branches.clear();
    }
    void recordIncident(PathAnalyserIncident incident) {
        currentBranch_->certainIncidents.emplace_back(incident);
        currentBranch_->potentialIncidents.emplace_back(incident);
    }
    void beginBranch() {
        currentBranch_->branches.emplace_back(Branch(currentBranch_));
        currentBranch_ = &currentBranch_->branches.back();
    }
    void endBranch() {
        assert(currentBranch_->branches.empty());
        currentBranch_ = currentBranch_->parent;
    }
    bool hasCertainly(PathAnalyserIncident incident) {
        return std::find(currentBranch_->certainIncidents.begin(), currentBranch_->certainIncidents.end(),
                         incident) != currentBranch_->certainIncidents.end();
    }
    bool hasPotentially(PathAnalyserIncident incident) {
        return std::find(currentBranch_->potentialIncidents.begin(), currentBranch_->potentialIncidents.end(),
                         incident) != currentBranch_->potentialIncidents.end();
    }
private:
    void copyPotentialIncidents() {
        for (auto branch : currentBranch_->branches) {
            currentBranch_->potentialIncidents.insert(currentBranch_->potentialIncidents.begin(),
                                                      branch.potentialIncidents.begin(),
                                                      branch.potentialIncidents.end());
        }
    }
    Branch mainBranch_ = Branch(nullptr);
    Branch *currentBranch_ = &mainBranch_;
};

}  // namespace EmojicodeCompiler

#endif /* PathAnalyser_hpp */
