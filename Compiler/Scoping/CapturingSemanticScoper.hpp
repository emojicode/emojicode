//
//  CapturingCallableScoper.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 27/12/2016.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#ifndef CapturingCallableScoper_hpp
#define CapturingCallableScoper_hpp

#include "SemanticScoper.hpp"
#include <utility>
#include <vector>

namespace EmojicodeCompiler {

struct VariableCapture {
    VariableCapture(VariableID id, Type type, VariableID captureId) : sourceId(id), type(std::move(type)), captureId(captureId) {}
    VariableID sourceId;
    Type type;
    /// The ID of the variable copy in the Closure
    VariableID captureId;
};

/** A @c CapturingCallableScoper can automatically capture unknown variables from another scope. These two
    scopes must share the same instance scope as capturing from instance scopes is not supported. */
class CapturingSemanticScoper : public SemanticScoper {
public:
    explicit CapturingSemanticScoper(SemanticScoper &captured)
    : SemanticScoper(captured.instanceScope()), capturedScoper_(captured) {}

    Scope& pushArgumentsScope(const std::vector<Parameter> &arguments, const SourcePosition &p) override {
        auto &scope = SemanticScoper::pushArgumentsScope(arguments, p);
        captureId_ = scope.reserveIds(capturedScoper_.currentScope().maxVariableId());
        return scope;
    }

    ResolvedVariable getVariable(const std::u32string &name, const SourcePosition &errorPosition) override;
    const std::vector<VariableCapture>& captures() const { return captures_; }
private:
    SemanticScoper &capturedScoper_;
    std::vector<VariableCapture> captures_;
    size_t captureId_;
};

}  // namespace EmojicodeCompiler

#endif /* CapturingCallableScoper_hpp */
