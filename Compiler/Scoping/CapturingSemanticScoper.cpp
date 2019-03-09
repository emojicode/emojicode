//
//  CapturingCallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CapturingSemanticScoper.hpp"
#include "VariableNotFoundError.hpp"
#include "Analysis/PathAnalyser.hpp"
#include "Analysis/ExpressionAnalyser.hpp"

namespace EmojicodeCompiler {

ResolvedVariable CapturingSemanticScoper::getVariable(const std::u32string &name,
                                                      const SourcePosition &errorPosition) {
    try {
        return SemanticScoper::getVariable(name, errorPosition);
    }
    catch (VariableNotFoundError &e) {
        auto pair = analyser_->scoper().getVariable(name, errorPosition);
        auto &variable = pair.variable;
        auto &captureVariable = topmostLocalScope().declareVariableWithId(variable.name(), variable.type(),
                                                                          constantCaptures_,
                                                                          VariableID(captureId_++), errorPosition);
        captureVariable.setCaptured();
        if (analyser_->pathAnalyser().hasCertainly(PathAnalyserIncident(false, variable.id()))) {
            pathAnalyser_->recordForMainBranch(PathAnalyserIncident(false, captureVariable.id()));
        }
        captures_.emplace_back(VariableCapture(variable.id(), variable.type(), captureVariable.id()));
        return ResolvedVariable(captureVariable, false);
    }
}

CapturingSemanticScoper::CapturingSemanticScoper(ExpressionAnalyser *analyser, bool makeCapturesConstant)
    : SemanticScoper(analyser->scoper().instanceScope()), analyser_(analyser),
      constantCaptures_(makeCapturesConstant) {}

Scope& CapturingSemanticScoper::pushArgumentsScope(PathAnalyser *analyser, const std::vector<Parameter> &arguments,
                                                   const SourcePosition &p) {
    auto &scope = SemanticScoper::pushArgumentsScope(analyser, arguments, p);
    captureId_ = scope.reserveIds(analyser_->scoper().currentScope().maxVariableId());
    return scope;
}

}  // namespace EmojicodeCompiler
