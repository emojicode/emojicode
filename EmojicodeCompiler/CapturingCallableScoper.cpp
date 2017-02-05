//
//  CapturingCallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CapturingCallableScoper.hpp"
#include "VariableNotFoundError.hpp"

ResolvedVariable CapturingCallableScoper::getVariable(const EmojicodeString &name, SourcePosition errorPosition) {
    try {
        return CallableScoper::getVariable(name, errorPosition);
    }
    catch (VariableNotFoundError &e) {
        auto pair = capturedScoper_.getVariable(name, errorPosition);
        auto &variable = pair.variable;
        auto &captureVariable = topmostLocalScope().setLocalVariable(variable.name(), variable.type(),
                                                                     variable.frozen(), variable.position(),
                                                                     variable.initialized());
        captures_.push_back(VariableCapture(variable.id(), variable.type(), captureVariable.id()));
        captureSize_ += variable.type().size();
        return ResolvedVariable(captureVariable, false);
    }
}
