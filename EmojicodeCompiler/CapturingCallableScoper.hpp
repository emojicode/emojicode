//
//  CapturingCallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CapturingCallableScoper_hpp
#define CapturingCallableScoper_hpp

#include <vector>
#include "CallableScoper.hpp"

struct VariableCapture {
    VariableCapture(int id, Type type, int captureId) : id(id), type(type), captureId(captureId) {}
    int id;
    Type type;
    int captureId;
};

/** A @c CapturingCallableScoper can automatically capture unknown variables from another scope. These two
    scopes must share the same instance scope as capturing from instance scopes is not supported. */
class CapturingCallableScoper : public CallableScoper {
public:
    CapturingCallableScoper(CallableScoper &captured)
        : CallableScoper(captured.instanceScope()), capturedScoper_(captured) {}
    virtual std::pair<Variable&, bool> getVariable(const EmojicodeString &name, SourcePosition errorPosition) override;
    const std::vector<VariableCapture>& captures() const { return captures_; }
    int captureSize() const { return captureSize_; }
private:
    CallableScoper &capturedScoper_;
    std::vector<VariableCapture> captures_;
    int captureSize_ = 0;
};

#endif /* CapturingCallableScoper_hpp */
