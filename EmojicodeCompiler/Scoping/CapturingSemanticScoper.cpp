//
//  CapturingCallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CapturingSemanticScoper.hpp"
#include "VariableNotFoundError.hpp"

namespace EmojicodeCompiler {

ResolvedVariable CapturingSemanticScoper::getVariable(const EmojicodeString &name,
                                                      const SourcePosition &errorPosition) {
    try {
        return SemanticScoper::getVariable(name, errorPosition);
    }
    catch (VariableNotFoundError &e) {
        auto pair = capturedScoper_.getVariable(name, errorPosition);
        auto &variable = pair.variable;
        auto &captureVariable = topmostLocalScope().declareVariableWithId(variable.name(), variable.type(), true,
                                                                          VariableID(captureId_++), errorPosition);
        captureVariable.initialized_ = maxInitializationLevel();
        captures_.emplace_back(VariableCapture(variable.id(), variable.type(), captureVariable.id()));
        return ResolvedVariable(captureVariable, false);
    }
}

}  // namespace EmojicodeCompiler
