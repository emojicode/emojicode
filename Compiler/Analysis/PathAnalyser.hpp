//
//  PathAnalyser.hpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 04/07/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef PathAnalyser_hpp
#define PathAnalyser_hpp

#include <vector>
#include <set>
#include <cstddef>

namespace EmojicodeCompiler {

struct ResolvedVariable;
struct SourcePosition;

/// Describes an incident that can be recorded with PathAnalyser.
class PathAnalyserIncident {
public:
    enum IncidentType {
        /// The super initializer was called. Can only occur in an class initializer.
        CalledSuperInitializer,
        /// The method returned. Raising an error is also considered returning from the PathAnalyser perspective.
        Returned,
        /// $this$, as represented by ASTThis was used.
        UsedSelf,
        /// A local variable was initialized (set to a value).
        VariableInit,
        /// An instance variable was initialized (set to a value).
        InstanceVarInit,
    };

    /// Creates an incident from an IncidentType.
    /// @warning Do not use this initializer to create a VariableInit or InstanceVarInit incident.
    PathAnalyserIncident(IncidentType type) : type_(type) {}
    /// Create a variable initialization incident.
    PathAnalyserIncident(bool inInstance, size_t id) : type_(inInstance ? PathAnalyserIncident::InstanceVarInit :
                                                             PathAnalyserIncident::VariableInit), value_(id) {}

    inline bool operator<(const PathAnalyserIncident &rhs) const {
        if (type_ < rhs.type_) return true;
        if ((type_ == VariableInit && rhs.type_ == VariableInit) ||
            (type_ == InstanceVarInit && rhs.type_ == InstanceVarInit)) {
            return value_ < rhs.value_;
        }
        return false;
    }

private:
    IncidentType type_;
    size_t value_;
};

/// PathAnalyser is resposible for determining which incidents may occur in a function.
///
/// For each branch it analyses it can determine which incidents may (potentially) occur at runtime and which will
/// certainly occur.
class PathAnalyser {
    struct Branch {
        explicit Branch(Branch *parent) : parent(parent) {}

        Branch *parent;
        std::set<PathAnalyserIncident> certainIncidents;
        std::set<PathAnalyserIncident> potentialIncidents;
        std::vector<Branch> branches;
    };

public:
    /// Records the occurence of an incident in the current branch.
    void record(PathAnalyserIncident incident) {
        currentBranch_->certainIncidents.emplace(incident);
        currentBranch_->potentialIncidents.emplace(incident);
    }

    /// Must be called when a branch (i.e. a block with branching character) begins.
    void beginBranch() {
        currentBranch_->branches.emplace_back(Branch(currentBranch_));
        currentBranch_ = &currentBranch_->branches.back();
    }
    /// Must be called at the end of the branch (i.e. end of the block).
    void endBranch();

    /// Must be called after all branches of a control flow structure, for which it is certain that at least one of the
    /// branches is executed, have been analysed.
    void finishMutualExclusiveBranches();
    /// Must be called after all branches of a control flow structure, whose branches must not be executed at
    /// at runtime, have been analysed.
    void finishUncertainBranches() {
        copyPotentialIncidents();
        currentBranch_->branches.clear();
    }

    /// Throws an error if the variable is not initalized.
    /// @throws CompilerError
    void uninitalizedError(const ResolvedVariable &rvar, const SourcePosition &p) const;

    /// Determines if the incident described by `incident` has certainly occured until now.
    bool hasCertainly(PathAnalyserIncident incident) const;
    /// Determines if the incident described by `incident` has probably occured until now.
    /// If hasCertainly() returns true for the provided incident, so does this method.
    bool hasPotentially(PathAnalyserIncident incident) const;

    /// Records the occurence of an incident as if it occured on the main branch.
    void recordForMainBranch(PathAnalyserIncident incident) {
        mainBranch_.certainIncidents.emplace(incident);
        mainBranch_.potentialIncidents.emplace(incident);
    }

private:
    void copyPotentialIncidents();
    void copyCertainIncidents();

    Branch mainBranch_ = Branch(nullptr);
    Branch *currentBranch_ = &mainBranch_;
};

}  // namespace EmojicodeCompiler

#endif /* PathAnalyser_hpp */
