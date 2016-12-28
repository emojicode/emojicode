//
//  CapturingCallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "CapturingCallableScoper.hpp"
#include "VariableNotFoundErrorException.hpp"

std::pair<Variable&, bool> CapturingCallableScoper::getVariable(const EmojicodeString &name,
                                                                SourcePosition errorPosition) {
    try {
        return CallableScoper::getVariable(name, errorPosition);
    }
    catch (VariableNotFoundErrorException &e) {
        auto pair = capturedScoper_.getVariable(name, errorPosition);
        auto &variable = pair.first;
        auto &captureVariable = topmostLocalScope().setLocalVariable(variable.string_, variable.type, variable.frozen(),
                                                                     variable.position(), variable.initialized);
        captures_.push_back(VariableCapture(variable.id(), variable.type, captureVariable.id()));
        captureSize_ += variable.type.size();
        return std::pair<Variable&, bool>(captureVariable, false);
    }
}
