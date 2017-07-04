//
//  CapturingCallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CapturingCallableScoper.hpp"
#include "VariableNotFoundError.hpp"

namespace EmojicodeCompiler {

ResolvedVariable CapturingCallableScoper::getVariable(const EmojicodeString &name,
                                                      const SourcePosition &errorPosition) {
    try {
        return CallableScoper::getVariable(name, errorPosition);
    }
    catch (VariableNotFoundError &e) {
        auto pair = capturedScoper_.getVariable(name, errorPosition);
        auto &variable = pair.variable;
        auto &captureVariable = topmostLocalScope().setLocalVariableWithID(variable.name(), variable.type(), true,
                                                                           captureId_, variable.position());
        captureVariable.initialized_ = maxInitializationLevel();
        captureVariable.initializationPosition_ = 0;
        captures_.push_back(VariableCapture(variable.id(), variable.type(), captureId_));
        captureSize_ += variable.type().size();
        captureId_ += variable.type().size();
        return ResolvedVariable(captureVariable, false);
    }
}

void CapturingCallableScoper::postArgumentsHook() {
    captureId_ = reserveVariable(capturedScoper_.fullSize());
}

};  // namespace EmojicodeCompiler
