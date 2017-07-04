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

namespace EmojicodeCompiler {

struct VariableCapture {
    VariableCapture(int id, Type type, int captureId) : id(id), type(type), captureId(captureId) {}
    int id;
    Type type;
    /// The ID of the variable copy in the Closure
    int captureId;
};

/** A @c CapturingCallableScoper can automatically capture unknown variables from another scope. These two
    scopes must share the same instance scope as capturing from instance scopes is not supported. */
class CapturingCallableScoper : public CallableScoper {
public:
    explicit CapturingCallableScoper(CallableScoper &captured)
        : CallableScoper(captured.instanceScope()), capturedScoper_(captured) {}
    ResolvedVariable getVariable(const EmojicodeString &name, const SourcePosition &errorPosition) override;
    const std::vector<VariableCapture>& captures() const { return captures_; }
    size_t captureSize() const { return captureSize_; }

    void postArgumentsHook() override;
private:
    CallableScoper &capturedScoper_;
    std::vector<VariableCapture> captures_;
    size_t captureSize_ = 0;
    size_t captureId_;
};

};  // namespace EmojicodeCompiler

#endif /* CapturingCallableScoper_hpp */
